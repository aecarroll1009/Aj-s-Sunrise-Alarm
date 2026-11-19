# Working notes

Firmware, bring-up sketches and 3D-printed enclosure for AJ's Sunrise Alarm, a
bedside sunrise lamp and alarm clock built as an anniversary gift. Start with
`README.md` (overview, hardware, status), then `docs/BRING_UP.md` (the bench
checklist currently being worked through) and `cad/README.md` (enclosure).

## Where things stand (2026-11-18)
- Firmware complete and unit-tested; compiles for `arduino:avr:nano`.
- Enclosure printed; fit against the real parts not yet checked.
- All parts in hand. Hardware bring-up is next, starting at step 1 of
  `docs/BRING_UP.md`. Work through it one step at a time with Alex at the
  bench, and don't skip ahead until a step passes.
- Open items are listed under Status in `README.md` (perfboard size mismatch
  in the CAD, `TODO(hardware)` tuning).

## Hardware decisions made during parts selection
- 1 m / 60-LED strip, not the spec's 2 m / 120. Worst-case draw is about 4 A
  total, so the 5 V / 8 A supply and 5 A fuse need no firmware current cap.
- RTC is a ZS-042 DS3231 board with its charging resistor removed; plain
  CR2032. Its EEPROM shows up at 0x57 on the I²C scan.
- OLED is a 2.42" SSD1309 (`OLED_DRIVER` in `src/Config.h`).
- Soft-power button must be momentary normally-open: the firmware reacts to a
  press edge and treats LOW on D5 as pressed. Arming is a menu item, not a
  switch.
- Speaker is a 38 mm 4 Ω 3 W driven from the DFPlayer's SPK_1/SPK_2.

## Toolchain
- `arduino-cli`: `C:\Users\PC\AppData\Local\arduino-cli\arduino-cli.exe`, with
  the AVR core, Adafruit NeoPixel, U8g2, RTClib and DFRobotDFPlayerMini.
- Native tests: `/usr/bin/bash test/run_tests.sh`. Host g++ is w64devkit at
  `C:\Users\PC\tools\w64devkit\bin`. Call Git Bash's `bash` by full path;
  w64devkit's `bash` is busybox and fails on the script.
- CAD: `C:\Users\PC\tools\cad-venv\Scripts\python.exe` (build123d, pyvista);
  see `cad/README.md`.
- The original spec is the scanned PDF one folder up from the repo.

## Conventions
- Pure-logic modules (`SunriseCurve`, `LightEngine`, `AlarmSequence`,
  `UIMenu`) make no Arduino calls and are covered by `test/`. Hardware access
  stays in the `*Driver` and `Persistence` adapters.
- Google-style docstrings, few inline comments, and comments that describe
  the code as it is rather than its history.
- Git: prepare commit and push commands for Alex rather than running them,
  unless he says to run them. No AI-attribution trailers in commit messages.
- After firmware changes, run the tests and compile the main sketch and the
  bring-up sketches before calling it done.
