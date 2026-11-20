#include "Lamp.h"

namespace {

// Scales a channel by brightness, rounded.
uint8_t scale(uint8_t c, uint8_t brightness) {
  return (uint8_t)(((uint16_t)c * brightness + 127) / 255);
}

}  // namespace

void Lamp::setBrightness(int16_t b) {
  if (b < MIN_BRIGHTNESS) b = MIN_BRIGHTNESS;
  if (b > 255) b = 255;
  brightness_ = (uint8_t)b;
}

Rgbw Lamp::color() const {
  if (!on_) return Rgbw{0, 0, 0, 0};
  Rgbw c = COLOR_PRESETS[preset_].color;
  return Rgbw{scale(c.r, brightness_), scale(c.g, brightness_),
              scale(c.b, brightness_), scale(c.w, brightness_)};
}
