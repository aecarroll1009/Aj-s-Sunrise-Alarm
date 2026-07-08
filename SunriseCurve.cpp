#include "SunriseCurve.h"

namespace {

// Linear interpolate between two 0..255 channel values.
// a,b are endpoints; num/den is the fractional position (0..den) inside the segment.
uint8_t lerp8(uint8_t a, uint8_t b, uint16_t num, uint16_t den) {
  if (den == 0) return a;
  int32_t delta = (int32_t)b - (int32_t)a;
  int32_t v = (int32_t)a + (delta * (int32_t)num + (int32_t)den / 2) / (int32_t)den;
  if (v < 0)   v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

// Interpolate the raw (perceptual) RGBW color at a given progress permille.
Rgbw interpColor(uint16_t progress) {
  const SunriseKeyframe* kf = SUNRISE_KEYFRAMES;
  // Before the first keyframe: hold the first color.
  if (progress <= kf[0].progress) return kf[0].c;
  // Walk segments to find the bracket [i-1, i] containing progress.
  for (uint8_t i = 1; i < SUNRISE_KEYFRAME_COUNT; i++) {
    if (progress <= kf[i].progress) {
      uint16_t p0 = kf[i - 1].progress;
      uint16_t den = kf[i].progress - p0;
      uint16_t num = progress - p0;
      Rgbw r;
      r.r = lerp8(kf[i - 1].c.r, kf[i].c.r, num, den);
      r.g = lerp8(kf[i - 1].c.g, kf[i].c.g, num, den);
      r.b = lerp8(kf[i - 1].c.b, kf[i].c.b, num, den);
      r.w = lerp8(kf[i - 1].c.w, kf[i].c.w, num, den);
      return r;
    }
  }
  // At/after the last keyframe: hold the final color.
  return kf[SUNRISE_KEYFRAME_COUNT - 1].c;
}

// Quadratic perceptual->PWM shaping. A nonzero input never maps to 0, so the
// ramp starts at PWM 1 and never pops from black (spec SS 4.7).
uint8_t shape(uint8_t v) {
  if (v == 0) return 0;
  uint16_t out = v;
  for (uint8_t i = 1; i < SUNRISE_GAMMA; i++) {
    out = (uint16_t)(out * v) / 255;
  }
  return out < 1 ? 1 : (uint8_t)out;
}

} // namespace

namespace SunriseCurve {

SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs) {
  uint16_t progress;
  if (durationMs == 0 || elapsedMs >= durationMs) {
    progress = 1000;
  } else {
    progress = (uint16_t)(((uint64_t)elapsedMs * 1000ULL) / durationMs);
  }

  Rgbw raw = interpColor(progress);
  Rgbw shaped;
  shaped.r = shape(raw.r);
  shaped.g = shape(raw.g);
  shaped.b = shape(raw.b);
  shaped.w = shape(raw.w);

  SunriseSample s;
  s.color = shaped;
  s.progress = progress;
  s.complete = (progress >= 1000);
  return s;
}

} // namespace SunriseCurve
