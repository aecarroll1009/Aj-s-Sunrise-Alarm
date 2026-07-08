#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// ============================================================================
// StripDriver — SK6812 RGBW strip wrapper (spec SS 3.2, 4.1, 4, fix 1/3).
//
// The whole strip shows one color at a time (sunrise or manual lamp). show()
// masks interrupts ~3.8 ms and pauses millis(), so we ONLY show() when a pixel
// actually changed (dirty flag): a static light costs no interrupt time, which
// is what lets audio run cleanly at full brightness.
// ============================================================================

class StripDriver {
public:
  void begin();
  void setAll(Rgbw c);   // stage a fill; sets dirty only if the color changed
  void render();         // show() iff dirty
  bool dirty() const { return dirty_; }

private:
  Adafruit_NeoPixel strip_{LED_COUNT, PIN_STRIP_DIN, NEO_GRBW + NEO_KHZ800};
  Rgbw last_{0, 0, 0, 0};
  bool dirty_{false};
};
