import { defineConfig } from 'vitest/config';

export default defineConfig({
	test: {
		include: ['*.test.ts'],
		globalSetup: ['globalSetup.ts'],
		// A phone boot plus a real serial session is nothing like a unit test
		testTimeout: 180000,
		hookTimeout: 300000,
		// QEMU runs the phone on an instruction clock pinned to real time: two phones competing
		// for the CPU boot slower than the suite waits for
		fileParallelism: false,
	},
});
