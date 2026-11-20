#include "SunriseCurve.h"

namespace {

// Interpolates one channel from a to b.
//
// Args:
//   pos: position within the segment, 0..span.
//   span: segment length; 0 returns a.
uint8_t lerp(uint8_t a, uint8_t b, uint16_t pos, uint16_t span) {
  if (span == 0) return a;
  int32_t delta = (int32_t)b - a;
  return (uint8_t)(a + (delta * pos + (int32_t)span / 2) / span);
}

// Interpolates all four channels from a to b.
Rgbw lerpColor(Rgbw a, Rgbw b, uint16_t pos, uint16_t span) {
  return Rgbw{lerp(a.r, b.r, pos, span), lerp(a.g, b.g, pos, span),
              lerp(a.b, b.b, pos, span), lerp(a.w, b.w, pos, span)};
}

// Returns the unshaped keyframe color at a progress in permille.
Rgbw keyframeColor(uint16_t progress) {
  const SunriseKeyframe* kf = SUNRISE_KEYFRAMES;
  if (progress <= kf[0].progress) return kf[0].color;
  for (uint8_t i = 1; i < SUNRISE_KEYFRAME_COUNT; ++i) {
    if (progress <= kf[i].progress) {
      uint16_t span = kf[i].progress - kf[i - 1].progress;
      uint16_t pos  = progress - kf[i - 1].progress;
      return lerpColor(kf[i - 1].color, kf[i].color, pos, span);
    }
  }
  return kf[SUNRISE_KEYFRAME_COUNT - 1].color;
}

// Gamma-shapes one channel. Nonzero input stays at least 1, so the light
// never pops on from black.
uint8_t shape(uint8_t v) {
  if (v == 0) return 0;
  uint16_t out = v;
  for (uint8_t i = 1; i < SUNRISE_GAMMA; ++i) out = (uint16_t)(out * v) / 255;
  return out < 1 ? 1 : (uint8_t)out;
}

Rgbw shapeColor(Rgbw c) {
  return Rgbw{shape(c.r), shape(c.g), shape(c.b), shape(c.w)};
}

// Returns progress in permille, clamped to 0..1000. A zero duration is done.
uint16_t progressFor(uint32_t elapsedMs, uint32_t durationMs) {
  if (durationMs == 0 || elapsedMs >= durationMs) return 1000;
  return (uint16_t)(((uint64_t)elapsedMs * 1000ULL) / durationMs);
}

}  // namespace

namespace SunriseCurve {

SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs) {
  SunriseSample s;
  s.progress = progressFor(elapsedMs, durationMs);
  s.color    = shapeColor(keyframeColor(s.progress));
  s.complete = (s.progress >= 1000);
  return s;
}

}  // namespace SunriseCurve
