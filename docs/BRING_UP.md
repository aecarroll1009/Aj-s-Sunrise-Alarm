# AJ's Sunrise Alarm — Hardware Bring-Up Checklist

Everything the firmware needs is written and unit-tested; this is the order
to bring the hardware up in the fall. Distilled from the spec (§3.8 bring-up,
§4.11 firmware build order, §3 golden rules) so you don't need the scanned
PDF at the bench.

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
- Native tests: `bash test/run_tests.sh` (needs w64devkit's g++ — see project notes).
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
   expect to see 0x3C and 0x68. Then upload `bringup/rtc_set` to set the
   DS3231's time (comment out the `rtc.adjust(...)` line after the first
   run) and watch Alarm1 fire.
4. Encoder and button. Upload `bringup/encoder_test`. Turning clockwise
   should count up; if it counts down, swap CLK and DT. Confirm both the
   encoder push and the soft-power button print an event.
5. Strip. Upload `bringup/strip_test`. Start with head-end power, data, and
   the 1000 µF cap only, at low brightness, and confirm the pixel count and
   that R/G/B/W each read pure — a tinted white means the color order isn't
   `NEO_GRBW`. Then add tail-end power injection and run full white; confirm
   the far end still holds about 5 V and the Nano doesn't reset (if it
   resets, either the cap isn't doing its job or a ground is loose).
6. DFPlayer. Upload `bringup/dfplayer_test` with `/mp3/0001.mp3` on the
   microSD. Confirm audio plays and the BUSY pin reads "playing".
7. All together. Disconnect USB, connect the brick to the 5V pin, flash the
   real firmware (over USB with the brick feed removed), then run it from
   the brick.

## After hardware works — fill in the `src/Config.h` placeholders (§4.12)
These are marked `TODO(hardware)` in `src/Config.h`; tune them on the real lamp:
- Sunrise keyframes and duration — the RGBW waypoints and ramp length (§4.7).
  Watch the dim pre-dawn phase for visible stepping and adjust the early
  keyframes if it does.
- Color preset list — final names (15 characters or fewer) and RGBW values (§4.4/4.6).
- Song track and volume — `ALARM_TRACK` and `ALARM_VOLUME` (§3.6).
- OLED driver — `OLED_DRIVER` defaults to the SSD1309 (§3.5). If the image is
  shifted or garbled, try `U8G2_SSD1309_128X64_NONAME0_1_HW_I2C` in
  `src/OledDriver.h`; for a different panel, set `OLED_DRIVER` to
  `OLED_SSD1306` or `OLED_SH1106`.

## Correctness checks already in the firmware
- The alarm triggers off the DS3231 hardware flag, not a time comparison.
- The alarm time lives in the DS3231; armed, firedOnDate, preset, and
  brightness live in EEPROM. Both survive power loss, and a reboot mid-alarm
  resumes sounding.
- The strip's `show()` only runs when a pixel changed, and never while audio
  is playing — the light stays static at full brightness while the song plays.
- The OLED uses a page buffer, not a full framebuffer, and dims after being
  idle on HOME.
