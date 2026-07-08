# AJ's Sunrise Alarm — Hardware Bring-Up Checklist

Everything the firmware needs is written and unit-tested; this is the order to
bring the **hardware** up in the fall. Distilled from the spec (§3.8 bring-up,
§4.11 firmware build order, §3 golden rules) so you don't need the scanned PDF
at the bench.

## Golden rules (read first — §3)
- Strip is powered by the **5V/8A supply directly**, never through the Nano.
- **All grounds common**: supply, Nano, strip, DFPlayer, RTC, OLED.
- Never touch mains — you only work on the 5V side.
- Barrel jack is **center-positive**; wire everything **unplugged**, plug in last.
- **USB *or* brick-to-5V-pin, never both at once** (back-feed). Flash over USB with
  the brick feed to the 5V pin removed; run on the brick with USB unplugged.

## Toolchain (already installed on the home machine)
- `arduino-cli` at `C:\Users\PC\AppData\Local\arduino-cli\arduino-cli.exe` (AVR core +
  NeoPixel, U8g2, RTClib, DFRobotDFPlayerMini).
- Native tests: `bash test/run_tests.sh` (needs w64devkit g++ — see project notes).
- Compile the main sketch:
  `arduino-cli compile -b arduino:avr:nano .`
- Compile/upload a bring-up sketch, e.g.:
  `arduino-cli compile -b arduino:avr:nano bringup/i2c_scan`
  `arduino-cli upload  -b arduino:avr:nano -p COMx bringup/i2c_scan`

## Bring-up order — test as you go (§3.8)
Do these **in order**; don't move on until each passes.

1. **Power rails only.** No modules. Confirm barrel polarity; meter the +5V bus to
   GND bus ≈ 5.0 V; barrel center pin = +5 V. Inline 8–10 A fuse in place.
2. **Nano alone, USB only** (brick feed to the 5V pin disconnected). Upload the
   stock **Blink** example — confirms the board + toolchain + COM port.
3. **OLED + RTC** (shared I²C, SDA=A4/SCL=A5). Upload `bringup/i2c_scan` → expect
   **0x3C and 0x68**. Then `bringup/rtc_set` to set the DS3231 time (edit the
   `rtc.adjust(...)` line out after the first run) and watch Alarm1 fire.
4. **Encoder + button.** Upload `bringup/encoder_test`. Turn CW → count up (if it
   counts down, swap CLK/DT). Confirm encoder push and soft-power both print.
5. **Strip.** Upload `bringup/strip_test`. **HEAD power + data + 1000 µF cap only**
   first, at LOW brightness: confirm the pixel **count** and that R/G/B/W each
   read pure (tinted white ⇒ not `NEO_GRBW`). Then add **TAIL injection** and run
   full white — confirm the far end holds ~5 V and the **Nano doesn't reset**
   (if it does: cap not working or a loose ground).
6. **DFPlayer.** Upload `bringup/dfplayer_test` with `/mp3/0001.mp3` on the µSD.
   Confirm audio plays and the BUSY pin reads "playing".
7. **All together.** Disconnect USB, connect the brick to the 5V pin, flash the
   real firmware (USB with brick feed removed), then run on the brick.

## After hardware works — fill in the `Config.h` placeholders (§4.12)
These are marked `TODO(hardware)` in `Config.h`; tune them on the real lamp:
- **Sunrise keyframes + duration** — the RGBW waypoints and ramp length (§4.7).
  Watch the dim pre-dawn phase for visible stepping; adjust the early keyframes.
- **Color preset list** — final names (≤15 chars) and RGBW values (§4.4/4.6).
- **Song track + volume** — `ALARM_TRACK` / `ALARM_VOLUME` (§3.6).
- If the panel is an **SH1106**, flip `OLED_IS_SH1106` to 1 (§3.5).

## Sanity checks baked into the firmware
- Alarm triggers off the **DS3231 hardware flag**, not a time comparison.
- Alarm time is in the **DS3231**; armed/firedOnDate/preset/brightness in **EEPROM**
  — survives power loss. Reboot mid-alarm resumes SOUNDING.
- Strip `show()` only fires when a pixel changed, and never during audio (the light
  is static at full brightness while the song plays).
- OLED uses the **page buffer** (not a full framebuffer) and dims after idle on HOME.
