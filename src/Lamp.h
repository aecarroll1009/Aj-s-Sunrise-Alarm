#pragma once
#include <stdint.h>
#include "Tuning.h"
#include "Types.h"

// The everyday lamp: on/off, a color preset and a brightness (spec 4.2).
// Pure logic.
class Lamp {
 public:
  void togglePower() { on_ = !on_; }
  void turnOff()     { on_ = false; }
  bool isOn() const  { return on_; }

  // Sets brightness, clamped to MIN_BRIGHTNESS..255.
  void setBrightness(int16_t b);
  void adjustBrightness(int16_t delta) { setBrightness((int16_t)brightness_ + delta); }
  uint8_t brightness() const { return brightness_; }

  // Selects a preset; out-of-range indexes wrap.
  void setPreset(uint8_t index) { preset_ = index % COLOR_PRESET_COUNT; }
  uint8_t preset() const { return preset_; }

  // Returns the preset scaled by brightness, or black when off.
  Rgbw color() const;

 private:
  bool    on_ = false;
  uint8_t preset_ = 0;
  uint8_t brightness_ = DEFAULT_BRIGHTNESS;
};
