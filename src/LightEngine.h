#pragma once
#include <stdint.h>
#include "Config.h"

// What the strip should show (spec 4.2).
//
// Pure logic, no hardware calls or Arduino headers, so it's testable by
// test/test_light_engine.cpp without a board. Holds the manual-lamp state
// and produces the final RGBW color; StripDriver writes whatever
// outputColor() returns.
//
// States:
//   LIGHT_OFF  strip is dark.
//   MANUAL     solid preset color scaled by a remembered brightness.
//   SUNRISE    the alarm owns the light. This is an authoritative override:
//              outputColor() returns whatever color AlarmSequence pushes in
//              each frame, whether that's the ramping sunrise color or full
//              brightness while sounding or snoozed.
//
// The soft-power button toggles LIGHT_OFF and MANUAL. On the home screen
// with the lamp on, the encoder adjusts brightness. Preset and brightness
// survive power-off: they're remembered here and persisted to EEPROM by the
// Persistence adapter.
class LightEngine {
public:
  enum State : uint8_t { LIGHT_OFF, MANUAL, SUNRISE };

  // Constructs a light that boots off, at the default brightness and preset.
  LightEngine();

  // Toggles between off and manual-on. Ignored while the alarm override is
  // active; the alarm owns the light until it releases it.
  void togglePower();

  // Sets brightness directly.
  //
  // Args:
  //   b: target brightness, clamped to [MANUAL_BRIGHTNESS_MIN, 255].
  void setBrightness(uint8_t b);

  // Adjusts brightness by a relative amount, e.g. from the encoder on HOME.
  //
  // Args:
  //   d: signed delta in brightness units.
  void adjustBrightness(int16_t d);

  // Sets the active color preset directly.
  //
  // Args:
  //   index: preset index, wrapped into range.
  void setPreset(uint8_t index);

  // Scrolls the active color preset, e.g. from the Color Preset menu.
  //
  // Args:
  //   d: signed step count; wraps at either end of the preset list.
  void nextPreset(int16_t d);

  // Enters the alarm override (spec 4.8). Remembers the current preset and
  // brightness so they can resume once the override ends.
  void beginOverride();

  // Sets the color shown while the override is active.
  //
  // Args:
  //   c: the color to display this frame, pushed by AlarmSequence.
  void setOverrideColor(Rgbw c);

  // Ends the override with the light off, e.g. on dismiss or cancel.
  void endOverrideOff();

  // Restores preset and brightness from persisted storage at boot.
  //
  // Args:
  //   presetIndex: saved preset index, wrapped into range.
  //   brightness: saved brightness, clamped to the valid range.
  void restore(uint8_t presetIndex, uint8_t brightness);

  State   state() const        { return state_; }
  bool    isOn() const         { return state_ == MANUAL || state_ == SUNRISE; }
  bool    isOverridden() const { return state_ == SUNRISE; }
  uint8_t brightness() const   { return brightness_; }
  uint8_t presetIndex() const  { return presetIndex_; }

  // Returns the final RGBW color to write to the strip for the current state.
  Rgbw outputColor() const;

private:
  State   state_;
  uint8_t presetIndex_;
  uint8_t brightness_;
  Rgbw    overrideColor_;
};
