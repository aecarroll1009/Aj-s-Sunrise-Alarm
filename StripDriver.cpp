#include "StripDriver.h"

namespace {
bool sameColor(const Rgbw& a, const Rgbw& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
}
}

void StripDriver::begin() {
  strip_.begin();
  strip_.clear();
  strip_.show();          // start dark
  last_  = Rgbw{0, 0, 0, 0};
  dirty_ = false;
}

void StripDriver::setAll(Rgbw c) {
  if (sameColor(c, last_)) return;      // no change -> stay clean, skip show()
  uint32_t packed = strip_.Color(c.r, c.g, c.b, c.w);
  for (uint16_t i = 0; i < LED_COUNT; ++i) strip_.setPixelColor(i, packed);
  last_  = c;
  dirty_ = true;
}

void StripDriver::render() {
  if (!dirty_) return;
  strip_.show();
  dirty_ = false;
}
