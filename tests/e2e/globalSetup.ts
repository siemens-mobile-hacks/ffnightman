// QEMU opens a GTK window even when nothing looks at it, so the emulators need an X server.
// One Xvfb is started here for the whole run, unless the machine already has a display.

import { spawn, spawnSync, ChildProcess } from 'node:child_process';

let xvfb: ChildProcess | undefined;

// Xvfb runs in a process group of its own, which a Ctrl-C in the terminal does not reach
function stopXvfb(): void {
	if (!xvfb)
		return;
	try {
		process.kill(-xvfb.pid!, 'SIGTERM');
	} catch {
		xvfb.kill('SIGTERM');
	}
	xvfb = undefined;
}

export async function setup(): Promise<void> {
	if (process.env.DISPLAY)
		return;
	// stdout is null when `which` itself is missing
	if (!spawnSync('which', ['Xvfb'], { encoding: 'utf8' }).stdout?.trim()) {
		console.warn('No DISPLAY and no Xvfb: the emulators will not start. Install Xvfb or run under an X server.');
		return;
	}

	// -displayfd lets Xvfb pick a free display number and report it back
	const child = spawn('Xvfb', ['-displayfd', '3', '-screen', '0', '1280x1024x24', '-nolisten', 'tcp'], {
		detached: true,
		stdio: ['ignore', 'ignore', 'ignore', 'pipe'],
	});
	xvfb = child;
	// vitest's own exit listener ends the process, so this one has to go first
	process.prependOnceListener('exit', stopXvfb);

	const display = await new Promise<string>((resolve, reject) => {
		let out = '';
		child.stdio[3]!.on('data', (chunk: Buffer) => {
			out += chunk.toString();
			if (out.includes('\n'))
				resolve(out.trim());
		});
		child.once('error', reject);
		child.once('exit', (code) => reject(new Error(`Xvfb exited with ${code}`)));
	});

	process.env.DISPLAY = `:${display}`;
	console.log(`Xvfb running on DISPLAY=${process.env.DISPLAY}`);
}

export async function teardown(): Promise<void> {
	process.off('exit', stopXvfb);
	stopXvfb();
}
