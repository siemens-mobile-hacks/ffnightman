// Writes files into a fullflash with ffnightman, boots the phone on it and downloads the files over
// OBEX: the phone's own file system has to find them, and give them back unchanged.

import { afterAll, beforeAll, beforeEach, describe, expect, test } from 'vitest';
import { execFile } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { promisify } from 'node:util';
import { setTimeout as delay } from 'node:timers/promises';
import { AsyncSerialPort, OBEX } from '@sie-js/serial';
import { EmulatedPhone, FFNIGHTMAN, findFullflash, RunningPhone, startPhone, unavailable } from './emulator.js';

// How long the phone gets until its file system answers: the AT interpreter comes up long before
const READY_TIMEOUT_MS = 180000;

// The directory the suite writes into, below the phone's /Data
const DIR = 'Misc/ffnightman-e2e';

type TestFile = {
	// below /Data on the phone, and below the partition for ffnightman
	path: string;
	data: Buffer;
};

// Content that tells a shifted or truncated copy from the original
function pattern(size: number, seed: number): Buffer {
	const data = Buffer.alloc(size);
	for (let i = 0; i < size; i++)
		data[i] = (i * 31 + (i >> 8) + seed * 7) & 0xFF;
	return data;
}

function filesFor(phone: EmulatedPhone): TestFile[] {
	const files: TestFile[] = [
		{ path: `${DIR}/small.bin`, data: pattern(100, 1) },
		{ path: `${DIR}/empty.bin`, data: Buffer.alloc(0) },
		{ path: `${DIR}/one-part.bin`, data: pattern(phone.chunkSize, 2) },
		{ path: `${DIR}/two-parts.bin`, data: pattern(phone.chunkSize + 1, 3) },
		{ path: `${DIR}/big.bin`, data: pattern(50000, 4) },
		{ path: `${DIR}/replaced.bin`, data: pattern(3000, 6) },
		{ path: `${DIR}/sub/nested.txt`, data: Buffer.from('Written by ffnightman\r\n') },
		// next to what the firmware keeps there
		{ path: 'Misc/ffnightman-e2e.txt', data: Buffer.from('Written by ffnightman into a directory of the firmware\r\n') },
	];
	if (phone.unicodeNames)
		files.push({ path: `${DIR}/файл.txt`, data: Buffer.from('Written by ffnightman, with a name beyond ASCII\r\n') });
	return files;
}

// What was written and then removed again
const REMOVED = `${DIR}/removed.bin`;

async function ffnightman(args: string[]): Promise<string> {
	try {
		const { stdout } = await promisify(execFile)(FFNIGHTMAN, args, { maxBuffer: 64 * 1024 * 1024 });
		return stdout;
	} catch (e) {
		const error = e as Error & { stdout?: string; stderr?: string };
		throw new Error(`ffnightman ${args.join(' ')} failed: ${error.message}\n${error.stdout ?? ''}${error.stderr ?? ''}`);
	}
}

// Extracts the whole filesystem, and returns where the phone's /Data is
async function extract(phone: EmulatedPhone, fullflash: string, dir: string): Promise<string> {
	await ffnightman(['-o', '-p', dir, fullflash]);
	return path.join(dir, phone.partition);
}

// How much of what the phone sent is kept for the panic check
const RECEIVED_TAIL_SIZE = 4096;

// The last bytes the phone sent, tapped below the stream so that the protocols see the port
// unchanged. A phone that panics writes ">>EXIT<< ... FILE: ..." to its serial line and never
// answers again; the OBEX client skips that text as garbage, so only this tail says why.
function tapReceived(port: AsyncSerialPort): () => Buffer {
	const binding = (port.getParentPort() as any).port;
	const read = binding.read;
	let tail = Buffer.alloc(0);
	binding.read = async (buffer: Buffer, offset: number, length: number) => {
		const result = await read.call(binding, buffer, offset, length);
		tail = Buffer.concat([tail, buffer.subarray(offset, offset + result.bytesRead)]).subarray(-RECEIVED_TAIL_SIZE);
		return result;
	};
	return () => tail;
}

function findPanic(received: Buffer): string | undefined {
	const text = received.toString('latin1');
	const at = text.indexOf('>>EXIT<<');
	return at < 0 ? undefined : text.slice(at, at + 300).replace(/[^\x20-\x7e]+/g, ' ').trim();
}

export function writeSuite(name: string, phone: EmulatedPhone): void {
	const reason = unavailable(phone);
	const suite = reason ? describe.skip : describe;
	if (reason)
		console.warn(`Skipping the ffnightman e2e tests on ${name}: ${reason}`);

	suite(`ffnightman writes into the fullflash of the ${name}`, () => {
		const files = filesFor(phone);
		let workDir: string;
		let original: string;
		let fullflash: string;
		let running: RunningPhone | undefined;
		let obex: OBEX;
		let received: () => Buffer = () => Buffer.alloc(0);

		beforeAll(async () => {
			workDir = fs.mkdtempSync(path.join(os.tmpdir(), 'ffnightman-e2e-'));
			original = findFullflash(phone.fullflash)!;
			fullflash = path.join(workDir, phone.fullflash);

			const args = ['--mkdir', `${phone.partition}/${DIR}`, '--mkdir', `${phone.partition}/${DIR}/sub`];
			const put = (file: TestFile, local: string) => {
				fs.writeFileSync(local, file.data);
				args.push('--put', `${local}:${phone.partition}/${file.path}`);
			};
			put({ path: `${DIR}/replaced.bin`, data: pattern(7000, 5) }, path.join(workDir, 'replaced-before.bin'));
			files.forEach((file, i) => put(file, path.join(workDir, `file-${i}`)));
			put({ path: REMOVED, data: pattern(5000, 7) }, path.join(workDir, 'removed.bin'));
			args.push('--rm', `${phone.partition}/${REMOVED}`);

			await ffnightman([...args, '--out', fullflash, original]);

			running = await startPhone(phone, fullflash);
			received = tapReceived(running.port);
			obex = new OBEX(running.port);
			try {
				await waitUntilReady();
			} catch (e) {
				const panic = findPanic(received());
				const log = running.log();
				await running.stop();
				throw new Error(`${(e as Error).message}` +
					`${panic ? `\nThe phone panicked: ${panic}` : ''}` +
					`${log ? `\nEmulator output:\n${log}` : ''}`);
			}
		});

		// A session lost in one test is opened again, so that it fails that test only
		beforeEach(async () => {
			if (running && !obex.isConnected)
				await obex.connect(0);
		});

		afterAll(async () => {
			await obex?.disconnect().catch(() => {});
			await running?.port.close().catch(() => {});
			await running?.stop();
			if (workDir)
				fs.rmSync(workDir, { recursive: true, force: true });
		});

		// The AT interpreter answers long before the file system behind OBEX is up: the phone
		// lists directories while opening a file in them is still answered with "Not found". So
		// this waits for a download to succeed.
		async function waitUntilReady(): Promise<void> {
			const deadline = Date.now() + READY_TIMEOUT_MS;
			let lastError: unknown;
			while (Date.now() < deadline) {
				try {
					if (!obex.isConnected)
						await obex.connect(0);
					await obex.getFile(`/Data/${phone.firmwareFile}`);
					return;
				} catch (e) {
					lastError = e;
					await delay(3000);
				}
			}
			throw new Error(`the phone was not ready after ${READY_TIMEOUT_MS / 1000} s: ${(lastError as Error)?.message}`);
		}

		test('boots the phone', () => {
			expect(obex.getDeviceName()).toBe(phone.deviceName);
			expect(obex.getPlatform()).toBe(phone.platform);
			expect(findPanic(received())).toBeUndefined();
		});

		test('lists the files and directories with their sizes', async () => {
			const listed = new Map<string, Map<string, { size: number; isDir: boolean }>>();
			const list = async (dir: string) => {
				if (!listed.has(dir)) {
					const entries = await obex.readDir(`/Data/${dir}`);
					listed.set(dir, new Map(entries.map((entry) => [entry.name, { size: entry.size, isDir: entry.isDir }])));
				}
				return listed.get(dir)!;
			};

			expect((await list('Misc')).get('ffnightman-e2e')?.isDir).toBe(true);
			expect((await list(DIR)).get('sub')?.isDir).toBe(true);

			for (const file of files) {
				const entry = (await list(path.posix.dirname(file.path))).get(path.posix.basename(file.path));
				expect(entry, file.path).toEqual({ size: file.data.length, isDir: false });
			}

			expect((await list(DIR)).has(path.posix.basename(REMOVED))).toBe(false);
		});

		test('downloads the files unchanged', async () => {
			for (const file of files) {
				const data = await obex.getFile(`/Data/${file.path}`).catch((e: Error) => {
					throw new Error(`${file.path}: ${e.message}`, { cause: e });
				});
				expect(data.equals(file.data), `${file.path}: ${data.length} bytes, expected ${file.data.length}`).toBe(true);
			}
		});

		test('downloads a file of the firmware unchanged', async () => {
			const dataDir = await extract(phone, original, path.join(workDir, 'original'));
			const expected = fs.readFileSync(path.join(dataDir, phone.firmwareFile));
			const data = await obex.getFile(`/Data/${phone.firmwareFile}`);
			expect(data.equals(expected), `${data.length} bytes, expected ${expected.length}`).toBe(true);
			expect(findPanic(received())).toBeUndefined();
		});

		// The phone writes to its flash while it runs, and may move the records around
		test('keeps the files readable after the phone ran on the flash', async () => {
			await obex.disconnect().catch(() => {});
			await running!.port.close().catch(() => {});
			await running!.stop();
			running = undefined;

			const dataDir = await extract(phone, fullflash, path.join(workDir, 'after'));
			for (const file of files) {
				const data = fs.readFileSync(path.join(dataDir, file.path));
				expect(data.equals(file.data), `${file.path}: ${data.length} bytes, expected ${file.data.length}`).toBe(true);
			}
			expect(fs.existsSync(path.join(dataDir, REMOVED))).toBe(false);
		});
	});
}
