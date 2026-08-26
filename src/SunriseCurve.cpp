#include "SunriseCurve.h"

namespace {

// Linearly interpolates one 0..255 channel value between two endpoints.
//
// Args:
//   a: value at the start of the segment.
//   b: value at the end of the segment.
//   num: position inside the segment, in the same units as den.
//   den: length of the segment; 0 returns a unchanged.
// Returns:
//   The interpolated value, clamped to 0..255.
uint8_t lerp8(uint8_t a, uint8_t b, uint16_t num, uint16_t den) {
  if (den == 0) return a;
  int32_t delta = (int32_t)b - (int32_t)a;
  int32_t v = (int32_t)a + (delta * (int32_t)num + (int32_t)den / 2) / (int32_t)den;
  if (v < 0)   v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

// Finds the two keyframes surrounding a progress value.
//
// Args:
//   progress: ramp position in permille (0..1000).
//   loOut: set to the keyframe at or before progress.
//   hiOut: set to the keyframe at or after progress. Equal to loOut when
//       progress is outside the keyframe table.
void findSegment(uint16_t progress, const SunriseKeyframe** loOut,
                  const SunriseKeyframe** hiOut) {
  const SunriseKeyframe* kf = SUNRISE_KEYFRAMES;
  if (progress <= kf[0].progress) {
    *loOut = *hiOut = &kf[0];
    return;
  }
  for (uint8_t i = 1; i < SUNRISE_KEYFRAME_COUNT; i++) {
    if (progress <= kf[i].progress) {
      *loOut = &kf[i - 1];
      *hiOut = &kf[i];
      return;
    }
  }
  *loOut = *hiOut = &kf[SUNRISE_KEYFRAME_COUNT - 1];
}

// Interpolates the raw, perceptual RGBW color at a ramp position.
//
// Args:
//   progress: ramp position in permille (0..1000).
// Returns:
//   The interpolated color, before gamma shaping.
Rgbw interpColor(uint16_t progress) {
  const SunriseKeyframe *lo, *hi;
  findSegment(progress, &lo, &hi);

  uint16_t den = hi->progress - lo->progress;
  uint16_t num = progress - lo->progress;
  Rgbw r;
  r.r = lerp8(lo->c.r, hi->c.r, num, den);
  r.g = lerp8(lo->c.g, hi->c.g, num, den);
  r.b = lerp8(lo->c.b, hi->c.b, num, den);
  r.w = lerp8(lo->c.w, hi->c.w, num, den);
  return r;
}

// Applies perceptual-to-PWM gamma shaping to one channel.
//
// A nonzero input never maps to 0, so the ramp starts at PWM 1 and never
// pops from black (spec 4.7).
//
// Args:
//   v: perceptual channel value, 0..255.
// Returns:
//   The shaped PWM value.
uint8_t shape(uint8_t v) {
  if (v == 0) return 0;
  uint16_t out = v;
  for (uint8_t i = 1; i < SUNRISE_GAMMA; i++) {
    out = (uint16_t)(out * v) / 255;
  }
  return out < 1 ? 1 : (uint8_t)out;
}

// Applies gamma shaping to all four channels of a color.
//
// Args:
//   raw: perceptual color, before shaping.
// Returns:
//   The shaped color, ready to write to the strip.
Rgbw shapeColor(Rgbw raw) {
  Rgbw shaped;
  shaped.r = shape(raw.r);
  shaped.g = shape(raw.g);
  shaped.b = shape(raw.b);
  shaped.w = shape(raw.w);
  return shaped;
}

// Converts an elapsed time into ramp progress.
//
// Args:
//   elapsedMs: milliseconds since the sunrise began.
//   durationMs: total ramp length; 0 means already complete.
// Returns:
//   Progress in permille (0..1000), clamped to the ramp's endpoints.
uint16_t progressFor(uint32_t elapsedMs, uint32_t durationMs) {
  if (durationMs == 0 || elapsedMs >= durationMs) return 1000;
  return (uint16_t)(((uint64_t)elapsedMs * 1000ULL) / durationMs);
}

} // namespace

namespace SunriseCurve {

SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs) {
  SunriseSample s;
  s.progress = progressFor(elapsedMs, durationMs);
  s.color = shapeColor(interpColor(s.progress));
  s.complete = (s.progress >= 1000);
  return s;
}

} // namespace SunriseCurve
