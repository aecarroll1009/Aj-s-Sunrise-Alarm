# Aj's Sunrise Alarm Clock

A bedside lamp that wakes you with a simulated sunrise — a slow RGBW light ramp
over ~20–30 minutes ending in a song. It's two products in one enclosure: an
alarm clock with a fixed sunrise curve, and a normal lamp with solid
preset colors and adjustable brightness. The alarm is always armed in the
background and is authoritative — once it starts it overrides whatever the lamp
is doing, so you can never silently miss it.

Firmware for an Arduino Nano (ATmega328P). Built as a non-blocking state machine
— no `delay()`; everything is timed off `millis()` and the RTC.

## Hardware

| Part | Notes |
|---|---|
| Arduino Nano (ATmega328P) | controller |
| SK6812 RGBW strip, 1 m / 60 px | dedicated white channel; wrapped around the LED core inside a translucent shade |
| DS3231 RTC (ZS-042 board) + CR2032 | battery-backed time and hardware alarm. The board's charging resistor ("201") has been removed so a non-rechargeable CR2032 is safe. |
| 2.42" OLED 128×64 (SSD1309, I²C) | clock and menu display |
| DFPlayer Mini + 38 mm 4 Ω 3 W speaker | wake song from microSD (FAT32, 32 GB or smaller) |
| Rotary encoder with push | menu UI, brightness, snooze |
| Momentary pushbutton, normally open, 10 mm panel mount | soft power: lamp on/off and alarm dismiss. Must be momentary N.O.; a latching or N.C. switch won't work. |
| 5 V / 8 A supply (ALITOVE), 5.5×2.5 panel jack, 5 A inline blade fuse, power injection at both strip ends | see `docs/BRING_UP.md` |

Full pin map is in `src/Config.h`; wiring and power notes are in `docs/BRING_UP.md`.
The 3D-printed enclosure is in `cad/`. The original spec is the scanned PDF one
folder up (`..\AJ's Sunrise Alarm B_W Scan.pdf`).

Power budget: 60 LEDs × 60 mA (all four channels full) is 3.6 A worst case,
plus about 0.5 A for the Nano, OLED and DFPlayer, so the 8 A supply and 5 A
fuse have headroom and the firmware needs no current cap.

## Firmware architecture

Pure logic is split from hardware so the state machines can be unit-tested
natively (no board required):

- Pure logic (`SunriseCurve`, `LightEngine`, `AlarmSequence`, `UIMenu`) —
  no Arduino calls; exercised by a host g++ test harness.
- Hardware adapters (`StripDriver`, `OledDriver`, `RtcDriver`,
  `DFPlayerDriver`, `InputDriver`, `Persistence`) — thin wrappers over the
  libraries.
- `AJs_Sunrise_Alarm.ino` — wires them into the main loop.

Key design guarantees:
- The alarm triggers off the DS3231 hardware flag, not a time comparison.
- Armed, firedOnDate, preset, and brightness persist to EEPROM, so the alarm
  survives power loss; a reboot mid-alarm resumes sounding.
- The OLED uses a page buffer, not a full framebuffer, to fit the 2 KB SRAM.
- The LED `show()` never runs while audio is playing.

## Build

Uses `arduino-cli` with the AVR core and Adafruit NeoPixel, U8g2, RTClib, and
DFRobotDFPlayerMini.

```sh
# compile the firmware for the Nano
arduino-cli compile -b arduino:avr:nano .

# upload (USB; brick feed to the 5V pin removed — see docs/BRING_UP.md)
arduino-cli upload -b arduino:avr:nano -p COMx .
```

## Test

The pure-logic modules have a native g++ test harness — no hardware needed:

```sh
/usr/bin/bash test/run_tests.sh
```

Use Git Bash's `bash` explicitly. If w64devkit's `bin` is first on `PATH`,
plain `bash` runs w64devkit's busybox shell, which fails with "bad
substitution".

## Layout

```
AJs_Sunrise_Alarm.ino   main sketch (non-blocking loop); includes from src/
src/                    firmware modules
  Config.h                pin map + hardware tunables/placeholders
  SunriseCurve.*          fixed wake-up light ramp        (pure)
  LightEngine.*           lamp state + alarm override     (pure)
  AlarmSequence.*         wake state machine              (pure)
  UIMenu.*                screen/menu/field-edit machine  (pure)
  *Driver.* / Persistence.*  hardware adapters
test/                   native unit tests + run_tests.sh
bringup/                standalone hardware diagnostic sketches
docs/BRING_UP.md        bench checklist for assembly + tuning
cad/                    parametric enclosure (build123d) and print-ready STEP files
CLAUDE.md               working notes for picking the project up in a new session
```

## Status (2026-11-18)

- **Firmware:** complete. Compiles for the Nano (about 90% flash, about
  900 B SRAM free) and all four native test suites pass.
- **Enclosure:** printed from `cad/to_print/`. Fit against the real parts has
  not been checked yet (see `cad/README.md`).
- **Parts:** all in hand.
- **Next:** hardware bring-up, starting at step 1 of `docs/BRING_UP.md`. No
  bring-up step has been run yet.
- **Open items:**
  - `DFPlayerDriver` calls `play(ALARM_TRACK)`, which plays the Nth file in
    the order files were copied onto the card, not `/mp3/0001.mp3`. It works
    if the song is the only file on the card. Switching to
    `playMp3Folder(ALARM_TRACK)` (driver and `bringup/dfplayer_test`) would
    make it play exactly `/mp3/0001.mp3`. Not changed yet.
  - The `TODO(hardware)` placeholders in `src/Config.h` (sunrise keyframes and
    duration, preset list, song track and volume) get tuned on the real lamp
    after bring-up.
