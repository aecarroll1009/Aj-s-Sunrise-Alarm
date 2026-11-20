#pragma once
#include <Adafruit_NeoPixel.h>
#include "Pins.h"
#include "Types.h"

// SK6812 RGBW strip, one color across all pixels (spec 3.2, 4.1).
//
// A strip write blocks interrupts for about 2.5 ms, so it only happens when
// the color changes, and at most every 30 ms. A steady light then never
// disturbs the DFPlayer's serial link.
class StripDriver {
 public:
  void begin();

  // Shows color c, writing to the strip only if needed.
  void show(Rgbw c, uint32_t nowMs);

 private:
  Adafruit_NeoPixel strip_{LED_COUNT, PIN_STRIP_DIN, NEO_GRBW + NEO_KHZ800};
  Rgbw     shown_{0, 0, 0, 0};
  uint32_t lastWriteMs_ = 0;
};
