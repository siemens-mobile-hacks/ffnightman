// Runs an emulated Siemens phone on a fullflash.
//
// pmb887x-emu boots the fullflash and exposes the phone's USART0 as a QEMU TCP chardev, which
// serialport reaches through serialport-bindings-socket. The firmware that answers there is the
// phone's own: its AT interpreter, its file system and its OBEX server.

import { spawn, spawnSync, ChildProcess } from 'node:child_process';
import fs from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as delay } from 'node:timers/promises';
import { SerialPortStream } from '@serialport/stream';
import { SocketBinding, SocketBindingInterface } from 'serialport-bindings-socket';
import { AsyncSerialPort, PhonePlatform } from '@sie-js/serial';

export type EmulatedPhone = {
	// the pmb887x-emu board name
	device: string;
	// the fullflash file in the fullflashes directory
	fullflash: string;
	// what the phone answers to AT+CGMI/AT+CGMM/AT+CGMR, and its platform as the OBEX client sees it
	deviceName: string;
	platform: PhonePlatform;
	// the partition ffnightman lists, that the phone shows as /Data
	partition: string;
	// the size of a file part on that partition: files longer than it take several
	chunkSize: number;
	// file names beyond ASCII: NewSGOLD keeps UTF-16 names, SGOLD 8-bit ones of no known encoding
	unicodeNames: boolean;
	// a file of the firmware in /Data, several parts long, that the phone leaves alone
	firmwareFile: string;
};

// The AT interpreter answers after ~10-16 s, but the rest of the startup then keeps blocking the
// serial task for seconds at a time, long enough for an OBEX exchange to time out. Waiting the
// boot out is what makes the suite reproducible.
const BOOT_WAIT_MS = 60000;

const HERE = path.dirname(fileURLToPath(import.meta.url));

// Either point the environment variables at them, or put them where the gitignored defaults are
const EMU_CANDIDATES = [
	process.env.FFNIGHTMAN_E2E_EMU,
	path.join(HERE, '.emu/pmb887x-emu'),
].filter((candidate): candidate is string => !!candidate);

const FULLFLASH_DIRS = [
	process.env.FFNIGHTMAN_E2E_FULLFLASHES,
	path.join(HERE, 'fullflashes'),
].filter((candidate): candidate is string => !!candidate);

export const FFNIGHTMAN = process.env.FFNIGHTMAN_E2E_BIN ?? path.join(HERE, '../../build/ffnightman');

function findEmulator(): string | undefined {
	for (const candidate of EMU_CANDIDATES) {
		if (fs.existsSync(candidate) && spawnSync(candidate, ['--version']).status === 0)
			return candidate;
	}
	// stdout is null when `which` itself is missing
	const fromPath = spawnSync('which', ['pmb887x-emu'], { encoding: 'utf8' }).stdout?.trim();
	return fromPath || undefined;
}

export function findFullflash(name: string): string | undefined {
	for (const dir of FULLFLASH_DIRS) {
		const file = path.join(dir, name);
		if (fs.existsSync(file))
			return file;
	}
	return undefined;
}

/**
 * Why the suite cannot run here, or undefined when it can. The suites skip themselves with this
 * message instead of failing: a checkout without the emulator and the fullflashes is the normal case.
 */
export function unavailable(phone: EmulatedPhone): string | undefined {
	if (!fs.existsSync(FFNIGHTMAN))
		return `ffnightman not found at ${FFNIGHTMAN}. Build it, or point FFNIGHTMAN_E2E_BIN at it.`;
	if (!findEmulator())
		return `pmb887x-emu not found (looked at ${EMU_CANDIDATES.join(', ')}, and in PATH). ` +
			`Build it from https://github.com/siemens-mobile-hacks/pmb887x-emu and point FFNIGHTMAN_E2E_EMU at it.`;
	if (!findFullflash(phone.fullflash))
		return `the fullflash ${phone.fullflash} was not found in ${FULLFLASH_DIRS.join(', ')}. ` +
			`Point FFNIGHTMAN_E2E_FULLFLASHES at a directory holding it.`;
	return undefined;
}

export interface RunningPhone {
	port: AsyncSerialPort;
	// everything the emulator printed, for a failure message
	log(): string;
	stop(): Promise<void>;
}

// Every emulator that is still running, so that none survives this process: they are spawned
// detached (to be killable as a group) and would otherwise keep eating CPU, which makes the next
// run's phones miss their timing.
const runningEmulators = new Set<() => void>();

// A QEMU that crashed was killed by a signal, and has no exit code
function exited(emu: ChildProcess): boolean {
	return emu.exitCode !== null || emu.signalCode !== null;
}

function killEmulatorGroup(emu: ChildProcess): void {
	if (exited(emu))
		return;
	try {
		process.kill(-emu.pid!, 'SIGKILL');
	} catch {
		emu.kill('SIGKILL');
	}
}

function killRunningEmulators(): void {
	for (const kill of runningEmulators)
		kill();
	runningEmulators.clear();
}

process.on('exit', killRunningEmulators);
// A signal listener replaces Node's default of terminating, so after the cleanup the signal is
// raised again with the listener gone
for (const signal of ['SIGINT', 'SIGTERM'] as const) {
	process.once(signal, () => {
		killRunningEmulators();
		process.kill(process.pid, signal);
	});
}

async function allocatePort(): Promise<number> {
	return new Promise((resolve, reject) => {
		const server = net.createServer();
		server.once('error', reject);
		server.listen(0, '127.0.0.1', () => {
			const { port } = server.address() as net.AddressInfo;
			server.close(() => resolve(port));
		});
	});
}

async function waitForSerialPort(port: number, timeoutMS: number, emu: ChildProcess): Promise<void> {
	const deadline = Date.now() + timeoutMS;
	while (Date.now() < deadline) {
		const open = await new Promise<boolean>((resolve) => {
			const socket = net.connect({ host: '127.0.0.1', port });
			socket.once('connect', () => { socket.destroy(); resolve(true); });
			socket.once('error', () => resolve(false));
		});
		if (open)
			return;
		if (exited(emu))
			throw new Error(`the emulator exited with ${emu.exitCode ?? emu.signalCode} before opening its serial port`);
		await delay(300);
	}
	throw new Error(`the emulator serial port 127.0.0.1:${port} did not open within ${timeoutMS} ms`);
}

/**
 * Boots the phone on the fullflash and opens its serial port. The fullflash is mapped writable
 * (--rw): the phone writes to its flash while it runs, and the suite reads the result back.
 */
export async function startPhone(phone: EmulatedPhone, fullflash: string): Promise<RunningPhone> {
	const tcpPort = await allocatePort();
	const args = [
		'--device', phone.device,
		'--fullflash', fullflash,
		// the emulator's own defaults for the siemens-* boards
		'--siemens-esn=12345678',
		'--siemens-imei=490154203237518',
		'--serial', `tcp:127.0.0.1:${tcpPort},server=on,wait=off`,
		'--rw',
	];

	// detached: the emulator and its QEMU get their own process group, which stop() kills
	const emu = spawn(findEmulator()!, args, {
		detached: true,
		env: { ...process.env, QEMU_AUDIO_DRV: 'none' },
		stdio: ['ignore', 'pipe', 'pipe'],
	});
	const output: Buffer[] = [];
	emu.stdout.on('data', (chunk: Buffer) => output.push(chunk));
	emu.stderr.on('data', (chunk: Buffer) => output.push(chunk));
	const log = () => Buffer.concat(output).toString().trimEnd();

	const kill = () => killEmulatorGroup(emu);
	runningEmulators.add(kill);

	const stop = async (): Promise<void> => {
		runningEmulators.delete(kill);
		const wasRunning = !exited(emu);
		kill();
		if (wasRunning)
			await new Promise((resolve) => emu.once('exit', resolve));
	};

	try {
		await waitForSerialPort(tcpPort, 60000, emu);
		await delay(BOOT_WAIT_MS);
		if (exited(emu))
			throw new Error(`the emulator exited with ${emu.exitCode ?? emu.signalCode} while the phone booted`);

		// The host baud rate is meaningless on a TCP chardev
		const port = new AsyncSerialPort(new SerialPortStream<SocketBindingInterface>({
			binding: SocketBinding,
			path: `tcp://127.0.0.1:${tcpPort}`,
			baudRate: 115200,
			autoOpen: false,
		}));
		await port.open();
		return { port, log, stop };
	} catch (e) {
		await stop();
		const emuOutput = log();
		throw new Error(`${(e as Error).message}${emuOutput ? `\nEmulator output:\n${emuOutput}` : ''}`);
	}
}
