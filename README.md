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
| SK6812 RGBW strip, ~120 px | dedicated white channel; two-stage lampshade diffuser |
| DS3231 RTC + CR2032 | battery-backed time and hardware alarm |
| 2" OLED 128×64 (SSD1306/SH1106, I²C) | clock / menu display |
| DFPlayer Mini + speaker | wake song from microSD |
| Rotary encoder + soft-power button | menu UI + on/off |
| 5V / 8A supply, inline fuse, power injection both ends | see `BRING_UP.md` |

Full pin map is in `Config.h`; wiring and power notes are in `BRING_UP.md`.

## Firmware architecture

Pure logic is split from hardware so the state machines can be unit-tested
natively (no board required):

- Pure logic (`SunriseCurve`, `LightEngine`, `AlarmSequence`, `UIMenu`) —
  no Arduino calls; exercised by a host g++ test harness.
- Hardware adapters (`StripDriver`, `OledDriver`, `RtcDriver`,
  `DFPlayerDriver`, `InputDriver`, `Persistence`) — thin wrappers over the
  libraries.
- `AJs_Sunrise_Alarm.ino` — wires them into the main loop.

Key design guarantees: the alarm triggers off the DS3231 hardware flag (not a
time comparison); armed/firedOnDate/preset/brightness persist to EEPROM and
the alarm survives power loss (a reboot mid-alarm resumes sounding); the OLED uses
a page buffer (not a full framebuffer) to fit the 2 KB SRAM; and the LED
`show()` never runs while audio is playing.

## Build

Uses `arduino-cli` with the AVR core and Adafruit NeoPixel, U8g2, RTClib, and
DFRobotDFPlayerMini.

```sh
# compile the firmware for the Nano
arduino-cli compile -b arduino:avr:nano .

# upload (USB; brick feed to the 5V pin removed — see BRING_UP.md)
arduino-cli upload -b arduino:avr:nano -p COMx .
```

## Test

The pure-logic modules have a native g++ test harness — no hardware needed:

```sh
bash test/run_tests.sh
```

## Layout

```
AJs_Sunrise_Alarm.ino   main sketch (non-blocking loop)
Config.h                pin map + hardware tunables/placeholders
SunriseCurve.*          fixed wake-up light ramp        (pure)
LightEngine.*           lamp state + alarm override     (pure)
AlarmSequence.*         wake state machine              (pure)
UIMenu.*                screen/menu/field-edit machine  (pure)
*Driver.* / Persistence.*  hardware adapters
test/                   native unit tests + run_tests.sh
bringup/                standalone hardware diagnostic sketches
BRING_UP.md             bench checklist for assembly + tuning
```

## Status

Firmware is complete and compiles for the Nano; the pure-logic core is fully
unit-tested. Remaining work is hardware assembly and on-device tuning of the
`TODO(hardware)` placeholders in `Config.h` (sunrise keyframes/duration, final
preset list, song track/volume) — see `BRING_UP.md`.
