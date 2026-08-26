#include "LightEngine.h"

namespace {

// Clamps a brightness value to the manual lamp's valid range.
//
// Args:
//   b: requested brightness.
// Returns:
//   b clamped to [MANUAL_BRIGHTNESS_MIN, 255].
uint8_t clampBrightness(int16_t b) {
  if (b < MANUAL_BRIGHTNESS_MIN) return MANUAL_BRIGHTNESS_MIN;
  if (b > 255) return 255;
  return (uint8_t)b;
}

// Scales one 0..255 color channel by a brightness value, with rounding.
//
// Args:
//   c: channel value before scaling.
//   brightness: scale factor, 0..255.
// Returns:
//   The scaled channel value.
uint8_t scaleCh(uint8_t c, uint8_t brightness) {
  return (uint8_t)(((uint16_t)c * brightness + 127) / 255);
}

// Scales every channel of a color by a brightness value.
//
// Args:
//   base: color before scaling.
//   brightness: scale factor, 0..255.
// Returns:
//   The scaled color.
Rgbw scaleColor(Rgbw base, uint8_t brightness) {
  Rgbw out;
  out.r = scaleCh(base.r, brightness);
  out.g = scaleCh(base.g, brightness);
  out.b = scaleCh(base.b, brightness);
  out.w = scaleCh(base.w, brightness);
  return out;
}

// Wraps a signed index into [0, count) so scrolling can move either way.
//
// Args:
//   idx: signed index, possibly out of range.
//   count: number of valid slots; 0 always returns 0.
// Returns:
//   idx wrapped into [0, count).
uint8_t wrapIndex(int16_t idx, uint8_t count) {
  if (count == 0) return 0;
  int16_t m = idx % (int16_t)count;
  if (m < 0) m += count;
  return (uint8_t)m;
}

} // namespace

LightEngine::LightEngine()
    : state_(LIGHT_OFF),
      presetIndex_(0),
      brightness_(DEFAULT_BRIGHTNESS),
      overrideColor_{0, 0, 0, 0} {}

void LightEngine::togglePower() {
  if (state_ == SUNRISE) return;
  state_ = (state_ == LIGHT_OFF) ? MANUAL : LIGHT_OFF;
}

void LightEngine::setBrightness(uint8_t b) {
  brightness_ = clampBrightness((int16_t)b);
}

void LightEngine::adjustBrightness(int16_t d) {
  brightness_ = clampBrightness((int16_t)brightness_ + d);
}

void LightEngine::setPreset(uint8_t index) {
  presetIndex_ = wrapIndex((int16_t)index, COLOR_PRESET_COUNT);
}

void LightEngine::nextPreset(int16_t d) {
  presetIndex_ = wrapIndex((int16_t)presetIndex_ + d, COLOR_PRESET_COUNT);
}

void LightEngine::beginOverride() {
  state_ = SUNRISE;
}

void LightEngine::setOverrideColor(Rgbw c) {
  overrideColor_ = c;
}

void LightEngine::endOverrideOff() {
  state_ = LIGHT_OFF;
}

void LightEngine::restore(uint8_t presetIndex, uint8_t brightness) {
  presetIndex_ = wrapIndex((int16_t)presetIndex, COLOR_PRESET_COUNT);
  brightness_  = clampBrightness((int16_t)brightness);
}

Rgbw LightEngine::outputColor() const {
  switch (state_) {
    case SUNRISE:
      return overrideColor_;
    case MANUAL:
      return scaleColor(COLOR_PRESETS[presetIndex_].c, brightness_);
    case LIGHT_OFF:
    default:
      return Rgbw{0, 0, 0, 0};
  }
}
