#include "LightEngine.h"

namespace {

uint8_t clampBrightness(int16_t b) {
  if (b < MANUAL_BRIGHTNESS_MIN) return MANUAL_BRIGHTNESS_MIN;
  if (b > 255) return 255;
  return (uint8_t)b;
}

// Scale one channel by brightness (0..255). Rounded.
uint8_t scaleCh(uint8_t c, uint8_t brightness) {
  return (uint8_t)(((uint16_t)c * brightness + 127) / 255);
}

// Wrap an index into [0, count) for signed movement.
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
  if (state_ == SUNRISE) return;              // alarm owns the light; ignore
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
  state_ = SUNRISE;                           // preset_/brightness_ remembered
}

void LightEngine::setOverrideColor(Rgbw c) {
  overrideColor_ = c;
}

void LightEngine::endOverrideOff() {
  state_ = LIGHT_OFF;                          // dismiss/cancel -> dark
}

void LightEngine::restore(uint8_t presetIndex, uint8_t brightness) {
  presetIndex_ = wrapIndex((int16_t)presetIndex, COLOR_PRESET_COUNT);
  brightness_  = clampBrightness((int16_t)brightness);
}

Rgbw LightEngine::outputColor() const {
  switch (state_) {
    case SUNRISE:
      return overrideColor_;                  // already the shaped/full color
    case MANUAL: {
      Rgbw base = COLOR_PRESETS[presetIndex_].c;
      Rgbw out;
      out.r = scaleCh(base.r, brightness_);
      out.g = scaleCh(base.g, brightness_);
      out.b = scaleCh(base.b, brightness_);
      out.w = scaleCh(base.w, brightness_);
      return out;
    }
    case LIGHT_OFF:
    default:
      return Rgbw{0, 0, 0, 0};
  }
}
