#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// SK6812 RGBW strip wrapper (spec 3.2, 4.1, fix 1/3).
//
// The whole strip shows one color at a time, whether that's the sunrise
// ramp or the manual lamp. show() masks interrupts for about 3.8 ms and
// pauses millis(), so this only calls show() when a pixel actually changed
// (tracked with a dirty flag). A static light then costs no interrupt time,
// which is what lets audio run cleanly at full brightness.
class StripDriver {
public:
  void begin();

  // Stages a fill across every pixel. Sets the dirty flag only if the color
  // actually changed.
  //
  // Args:
  //   c: color to fill the strip with.
  void setAll(Rgbw c);

  // Calls show() if the staged color is dirty, then clears the flag.
  void render();

  bool dirty() const { return dirty_; }

private:
  Adafruit_NeoPixel strip_{LED_COUNT, PIN_STRIP_DIN, NEO_GRBW + NEO_KHZ800};
  Rgbw last_{0, 0, 0, 0};
  bool dirty_{false};
};
