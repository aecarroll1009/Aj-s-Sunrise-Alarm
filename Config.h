#pragma once
#include <stdint.h>

// ============================================================================
// Config.h — pin map, hardware constants, and bring-up placeholders.
//
// Everything the firmware needs to know about THIS board lives here. Values
// marked  TODO(hardware)  are the ones the spec (SS 4.12) defers to bring-up in
// Seattle: exact sunrise keyframes/duration, the final preset list, and the
// song track/volume. They are placeholders now so the firmware compiles and
// runs; tune them on real hardware.
// ============================================================================

// ---- Nano pin map (spec SS 3.7) -------------------------------------------
// A4/A5 are the I2C bus (Wire library owns them); listed for reference only.
constexpr uint8_t PIN_ENC_CLK   = 2;   // encoder CLK  — interrupt-capable
constexpr uint8_t PIN_ENC_DT    = 3;   // encoder DT   — interrupt-capable
constexpr uint8_t PIN_ENC_SW    = 4;   // encoder push — INPUT_PULLUP
constexpr uint8_t PIN_SOFT_PWR  = 5;   // soft power button — INPUT_PULLUP, active-low
constexpr uint8_t PIN_STRIP_DIN = 6;   // SK6812 data (via 330R)
constexpr uint8_t PIN_DF_RX     = 7;   // -> DFPlayer RX (Nano TX side, via 1k). SoftwareSerial TX
constexpr uint8_t PIN_DF_TX     = 8;   // <- DFPlayer TX.                         SoftwareSerial RX
constexpr uint8_t PIN_DF_BUSY   = 9;   // DFPlayer BUSY — plain input, playback status w/o serial
// A4 = SDA, A5 = SCL (OLED + DS3231 share the bus)

// ---- LED strip (spec SS 4.1) ----------------------------------------------
constexpr uint16_t LED_COUNT = 120;    // ~2 m of 60 LED/m around the ~5" inner shade
// Strip is SK6812 RGBW -> NEO_GRBW + NEO_KHZ800 when constructing Adafruit_NeoPixel.

// ---- I2C addresses (spec SS 3.5) ------------------------------------------
constexpr uint8_t I2C_ADDR_OLED = 0x3C;
constexpr uint8_t I2C_ADDR_RTC  = 0x68;

// ---- OLED chip select -----------------------------------------------------
// Flip to 1 if the panel is actually an SH1106 (one-line U8g2 constructor swap,
// spec SS 3.5/4.1). Kept as a single switch because the board is in Seattle.
#define OLED_IS_SH1106 0

// ---- Timing constants (spec SS 4.3, 4.4, 4.6, 4.8) ------------------------
constexpr uint16_t OLED_IDLE_MS     = 12000;  // ~10-15 s idle on HOME -> dim/blank
constexpr uint32_t SNOOZE_MS        = 600000UL; // 10:00 snooze countdown
constexpr uint16_t EDIT_FLASH_MS    = 500;    // editing field blink period (steady = navigating)
constexpr uint16_t PREVIEW_MIN_MS   = 30;     // Color Preset: rate-limit show() to ~1 per 30 ms
constexpr uint16_t LONG_PRESS_MS    = 600;    // encoder press >= this -> long press (save/confirm-exit)
constexpr uint8_t  DEBOUNCE_MS      = 10;     // button debounce window

// ---- Manual lamp behavior (spec SS 4.2) -----------------------------------
constexpr uint8_t DEFAULT_BRIGHTNESS      = 200; // MANUAL brightness at first boot
constexpr uint8_t MANUAL_BRIGHTNESS_MIN   = 8;   // rotate-down floor; lamp stays lit (OFF is the power button)

// ---- RGBW color type ------------------------------------------------------
struct Rgbw { uint8_t r, g, b, w; };

// ---- Shared UI value types (spec SS 4.4-4.5; 12-hour clock throughout) -----
struct ClockHM { uint8_t hour12; uint8_t minute; bool pm; }; // hour12 in 1..12
struct CalDate { uint8_t month; uint8_t day; uint16_t year; };

constexpr uint8_t  UI_BRIGHTNESS_STEP = 12;   // brightness units per detent on HOME
constexpr uint16_t YEAR_MIN = 2024;           // Set Time year edit range
constexpr uint16_t YEAR_MAX = 2099;           // spec mentions a mis-dial to 2099

// ============================================================================
// TODO(hardware): sunrise curve — placeholder keyframes + duration (spec SS 4.7)
// ----------------------------------------------------------------------------
// Fixed, not user-editable. Keyframe + linear interpolation between RGBW
// waypoints; SunriseCurve applies the quadratic PWM shaping so perceived
// brightness looks linear. `progress` is permille (0..1000) of the ramp.
// Shape per SS 4.7: deep red -> orange/amber -> warm white -> bright white.
// Start at PWM 1 (never a hard black->pop). EXACT values are tuned on hardware.
// ============================================================================
constexpr uint32_t SUNRISE_DURATION_MS = 25UL * 60UL * 1000UL; // 25 min (spec: 20-30)

struct SunriseKeyframe { uint16_t progress; Rgbw c; }; // progress 0..1000

constexpr SunriseKeyframe SUNRISE_KEYFRAMES[] = {
  //  progress   R    G    B    W      phase (SS 4.7)
  {     0,  {   1,   0,   0,   0 } },  // pre-dawn start: dim deep red, PWM 1 (no black pop)
  {   200,  {  60,   3,   0,   0 } },  // pre-dawn end:   R only, very low
  {   500,  { 200,  70,   0,   0 } },  // dawn:           red -> orange -> amber, G climbing
  {   850,  { 255, 150,  20,  90 } },  // sunrise:        amber -> warm white, W climbing
  {  1000,  { 255, 180,  60, 255 } },  // day:            warm -> bright white, W dominant
};
constexpr uint8_t SUNRISE_KEYFRAME_COUNT =
    sizeof(SUNRISE_KEYFRAMES) / sizeof(SUNRISE_KEYFRAMES[0]);

// ============================================================================
// TODO(hardware): manual-lamp color presets (spec SS 4.4, 4.6, 4.12)
// ----------------------------------------------------------------------------
// Names must be <= 15 chars so the HOME "Lamp:" line doesn't clip. Final list
// chosen at home. Brightness is applied separately by the light engine.
// ============================================================================
struct ColorPreset { const char* name; Rgbw c; }; // name <= 15 chars

constexpr ColorPreset COLOR_PRESETS[] = {
  { "Warm White",  { 255, 170,  70, 255 } },
  { "Bright White",{ 255, 220, 180, 255 } },
  { "Deep Red",    { 255,   0,   0,   0 } },
  { "Amber",       { 255, 120,   0,   0 } },
  { "Ocean Blue",  {   0,  80, 255,   0 } },
  { "Forest",      {  20, 200,  40,   0 } },
};
constexpr uint8_t COLOR_PRESET_COUNT =
    sizeof(COLOR_PRESETS) / sizeof(COLOR_PRESETS[0]);

// ============================================================================
// TODO(hardware): audio — song track + volume (spec SS 3.6, 4.12)
// ----------------------------------------------------------------------------
// Song lives on the microSD as /mp3/0001.mp3. Volume is 0..30 (DFPlayer scale).
// ============================================================================
constexpr uint8_t ALARM_TRACK  = 1;   // -> /mp3/0001.mp3
constexpr uint8_t ALARM_VOLUME = 20;  // 0..30
