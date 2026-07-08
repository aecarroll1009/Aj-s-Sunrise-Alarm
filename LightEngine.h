#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// LightEngine — what the strip should show (spec SS 4.2).
//
// PURE LOGIC: no hardware calls, no Arduino headers. Holds the manual-lamp
// state and produces the final RGBW; StripDriver writes whatever outputColor()
// returns. Native-testable (test/test_light_engine.cpp).
//
// States:
//   LIGHT_OFF  strip dark.
//   MANUAL     solid preset color scaled by a remembered brightness.
//   SUNRISE    the alarm owns the light (authoritative override): outputColor()
//              returns the color the AlarmSequence pushes in each frame (the
//              ramping sunrise color, or full brightness while sounding/snoozed).
//
// The soft-power button toggles LIGHT_OFF <-> MANUAL. On HOME with the lamp on,
// the encoder adjusts brightness. Preset + brightness survive power-off (they
// are remembered here and persisted to EEPROM by the Persistence adapter).
// ============================================================================

class LightEngine {
public:
  enum State : uint8_t { LIGHT_OFF, MANUAL, SUNRISE };

  LightEngine();

  // --- manual controls (no-ops while the alarm override is active) ---------
  void togglePower();                 // soft power: LIGHT_OFF <-> MANUAL
  void setBrightness(uint8_t b);      // absolute, clamped to [MIN..255]
  void adjustBrightness(int16_t d);   // encoder on HOME (d in brightness units)
  void setPreset(uint8_t index);      // absolute (wraps)
  void nextPreset(int16_t d);         // Color Preset scroll (wraps, signed)

  // --- alarm override (authoritative, spec SS 4.8) -------------------------
  void beginOverride();               // remember state, enter SUNRISE
  void setOverrideColor(Rgbw c);      // alarm pushes the live color each frame
  void endOverrideOff();              // dismiss/cancel -> light OFF

  // --- persistence hook ----------------------------------------------------
  void restore(uint8_t presetIndex, uint8_t brightness); // from EEPROM at boot

  // --- queries -------------------------------------------------------------
  State   state() const       { return state_; }
  bool    isOn() const        { return state_ == MANUAL || state_ == SUNRISE; }
  bool    isOverridden() const{ return state_ == SUNRISE; }
  uint8_t brightness() const  { return brightness_; }
  uint8_t presetIndex() const { return presetIndex_; }
  Rgbw    outputColor() const;        // final RGBW to write to the strip

private:
  State   state_;
  uint8_t presetIndex_;
  uint8_t brightness_;
  Rgbw    overrideColor_;
};
