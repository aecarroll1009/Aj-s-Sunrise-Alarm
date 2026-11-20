# AJ's Sunrise Alarm — Hardware Bring-Up Checklist

Everything the firmware needs is written and unit-tested; this is the order
to bring the hardware up in. Distilled from the spec (§3.8 bring-up, §4.11
firmware build order, §3 golden rules) so you don't need the scanned PDF at
the bench.

As of 2026-11-18 all parts are in hand and the enclosure is printed. No step
below has been run yet.

## Before you start
- RTC: the ZS-042 DS3231 board's "201" charging resistor is already removed,
  so a plain CR2032 goes in the holder on the back. Wire it from the 6-pin
  header: GND, VCC, SDA, SCL. Leave SQW and 32K unconnected.
- microSD: 32 GB or smaller, FAT32 (or FAT16 for 2 GB and under). If it's a
  used or no-name card, run H2testw on it first. Create `mp3\0001.mp3` and
  other files on the card are ignored.
- Barrel jack: the supply's plug is 5.5×2.5 mm. Test-fit it in the panel jack
  before wiring; a 2.5 mm plug on a 2.1 mm pin is loose and can arc.

## Golden rules (read first — §3)
- The strip is powered directly by the 5V/8A supply, never through the Nano.
- All grounds are common: supply, Nano, strip, DFPlayer, RTC, and OLED.
- Never touch mains. Everything here stays on the 5V side.
- The barrel jack is center-positive. Wire everything unplugged, then plug it in last.
- Never connect USB and the brick's feed to the 5V pin at the same time —
  that back-feeds one supply into the other. Flash over USB with the brick
  feed removed; run on the brick with USB unplugged.

## Toolchain (already installed on the home machine)
- `arduino-cli` at `C:\Users\PC\AppData\Local\arduino-cli\arduino-cli.exe` (AVR core,
  plus NeoPixel, U8g2, RTClib, and DFRobotDFPlayerMini).
- Native tests: `/usr/bin/bash test/run_tests.sh` (needs w64devkit's g++ at
  `C:\Users\PC\tools\w64devkit\bin`; the script adds it to `PATH`. Call Git
  Bash's `bash` explicitly, since w64devkit's own `bash` is busybox and fails).
- Compile the main sketch:
  `arduino-cli compile -b arduino:avr:nano .`
- Compile and upload a bring-up sketch, e.g.:
  `arduino-cli compile -b arduino:avr:nano bringup/i2c_scan`
  `arduino-cli upload  -b arduino:avr:nano -p COMx bringup/i2c_scan`

## Bring-up order — test as you go (§3.8)
Do these in order. Don't move on until each one passes.

1. Power rails only, no modules connected. Confirm barrel polarity; meter the
   +5V bus to the ground bus at about 5.0 V, with the barrel center pin at
   +5V. Confirm the inline 5 A blade fuse is in place on the +5V lead.
2. Nano alone, USB only, with the brick feed to the 5V pin disconnected.
   Upload the stock Blink example to confirm the board, toolchain, and COM
   port all work.
3. OLED and RTC, sharing I²C (SDA=A4/SCL=A5). Before connecting the OLED,
   check its back for a small 3-pin regulator: if there is none, power it
   from the Nano's 3V3 pin instead of the 5V bus. Upload `bringup/i2c_scan` and
   expect to see 0x3C (OLED), 0x57 (the 24C32 EEPROM on the ZS-042 board,
   unused by the firmware) and 0x68 (DS3231). If the OLED shows up at 0x3D
   instead, the firmware needs `setI2CAddress(0x3D * 2)` on the U8g2 object
   before `begin()`. Then upload `bringup/rtc_set` to set the
   DS3231's time (comment out the `rtc.adjust(...)` line after the first
   run) and watch Alarm1 fire.
4. Encoder and button. Upload `bringup/encoder_test`. Turning clockwise
   should count up; if it counts down, swap CLK and DT. Confirm both the
   encoder push and the soft-power button print an event. The soft-power
   button is one leg to D5 and the other to GND, no resistor; it must be
   momentary normally-open (a normally-closed switch reads as permanently
   held).
5. Strip. Upload `bringup/strip_test`. Start with head-end power, data, and
   the 1000 µF cap only, at low brightness, and confirm the pixel count and
   that R/G/B/W each read pure — a tinted white means the color order isn't
   `NEO_GRBW`. Then add tail-end power injection and run full white; confirm
   the far end still holds about 5 V and the Nano doesn't reset (if it
   resets, either the cap isn't doing its job or a ground is loose).
6. DFPlayer. Upload `bringup/dfplayer_test` with `/mp3/0001.mp3` on the
   microSD. Confirm audio plays and the BUSY pin reads "playing". The speaker
   goes on SPK_1/SPK_2 (not DAC_L/DAC_R), and the 100 µF + 0.1 µF caps go at
   the module. The test sketch and the driver both
   play exactly `/mp3/0001.mp3` and start it again whenever BUSY goes idle,
   so the song repeats until the alarm is snoozed or dismissed. Confirm the
   serial output shows a new "start #" each time the song ends. BUSY must be
   wired: with D9 floating it reads idle and the track restarts every second.
7. All together. Disconnect USB, connect the brick to the 5V pin, flash the
   real firmware (over USB with the brick feed removed), then run it from
   the brick.

## After hardware works — fill in the `src/Tuning.h` placeholders (§4.12)
These are marked `TODO(hardware)` in `src/Tuning.h`; tune them on the real lamp:
- Sunrise keyframes and duration — the RGBW waypoints and ramp length (§4.7).
  Watch the dim pre-dawn phase for visible stepping and adjust the early
  keyframes if it does.
- Color preset list — final names (15 characters or fewer) and RGBW values (§4.4/4.6).
- Song track and volume — `ALARM_TRACK` and `ALARM_VOLUME` (§3.6).
- OLED driver — `OLED_DRIVER` in `src/Pins.h` defaults to the SSD1309 (§3.5).
  If the image is shifted or garbled, try
  `U8G2_SSD1309_128X64_NONAME0_1_HW_I2C` in `src/OledDriver.h`; for a
  different panel, set `OLED_DRIVER` to `OLED_SSD1306` or `OLED_SH1106`.
  Screen layouts and fonts are in `src/Screens.cpp`.

## Correctness checks already in the firmware
- The alarm triggers off the DS3231 hardware flag, not a time comparison.
- The alarm time lives in the DS3231; armed, the day the alarm last ran,
  preset and brightness live in EEPROM. Both survive power loss, and a reboot
  mid-alarm resumes sounding.
- The strip is only rewritten when its color changes (at most every 30 ms),
  so the light stays static at full brightness while the song plays.
- The OLED uses a page buffer, not a full framebuffer, and dims after being
  idle on HOME.
