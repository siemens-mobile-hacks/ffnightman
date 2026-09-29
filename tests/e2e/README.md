# ffnightman e2e tests

These tests check what ffnightman writes against the firmware that has to read it. Each suite
copies a fullflash, writes files and directories into it with `ffnightman --put/--mkdir/--rm`,
boots the phone on it in [pmb887x-emu](https://github.com/siemens-mobile-hacks/pmb887x-emu) and
talks to the phone's own OBEX server with the client of
[@sie-js/serial](https://github.com/siemens-mobile-hacks/node-sie-serial). They check that:

* the phone boots: it answers, names itself and does not panic,
* its listings show the directories and the files with their sizes, and not the removed file,
* the files download unchanged, and so does a file of the firmware that was not touched,
* after the phone has run on the flash — it writes to it while it runs — libffshit still reads
  the files back unchanged.

```
cmake --build ../../build          # ffnightman, against a libffshit with write support
pnpm install
pnpm test                          # every phone
pnpm test s75                      # one of them
```

| Variable | Default | |
| --- | --- | --- |
| `FFNIGHTMAN_E2E_BIN` | `../../build/ffnightman` | the ffnightman to test |
| `FFNIGHTMAN_E2E_EMU` | `.emu/pmb887x-emu` | the emulator, else `pmb887x-emu` from `PATH` |
| `FFNIGHTMAN_E2E_FULLFLASHES` | `fullflashes` | a directory holding the fullflashes |

`.emu` and `fullflashes` are gitignored: a symlink to an emulator build directory and one to a
fullflash collection do the job. Without them the suites skip themselves and say what is missing.
Without a `DISPLAY` the suite starts an `Xvfb`, which QEMU needs for its window.

## The phones

| Suite | Fullflash | Platform | |
| --- | --- | --- | --- |
| `el71` | `EL71v41lg91.bin` | SGOLD2_ELKA | |
| `s75` | `S75v40lg1.bin` | SGOLD2 | its data partition has no free block left: ffnightman has to reclaim space first |
| `cx70` | `CX70v56lg3.bin` | SGOLD | fails on the emulator, see below |
| `sl65` | `SL65v49lg1_TIM.bin` | SGOLD | fails on the emulator, see below |

The files go into `/Data/Misc/ffnightman-e2e`, one next to the firmware's in `/Data/Misc`. They
are small, empty, exactly the partition's chunk size and one byte over it, 50 KB long, replaced
by a second `--put`, in a subdirectory, and on the NewSGOLD phones named beyond ASCII. One more
is written and removed again.

Each suite boots its own phone, and waits a minute before talking to it: the AT interpreter
answers after 10-16 s, but the rest of the startup keeps the serial task busy for seconds at a
time. The suites run one at a time: QEMU pins the phone's instruction clock to real time, so
phones competing for the CPU, or a busy machine, boot slower than the suites wait for.

## The SGOLD phones

The emulated CX70 and SL65 lose serial commands, from their first minute on. The OBEX client
repeats a lost read, but when the repeat is lost too the session is gone, and the phone stays in
OBEX mode where the client's AT probes cannot reach it. A download of a few dozen packets
rarely makes it, whatever the file: on an untouched CX70 fullflash, the firmware's 50 KB
`/Data/System/T9/t9de.ldb` fails the same way `big.bin` does. So the SGOLD suites fail in
"downloads the files unchanged", while the smaller files download fine and the listings, which
the firmware computes by walking every part of every file, show the right sizes.
