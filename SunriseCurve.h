#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// SunriseCurve — the fixed wake-up light ramp (spec SS 4.7).
//
// PURE LOGIC: no hardware calls, no Arduino headers. Given the elapsed time
// since the sunrise started, it returns the RGBW color the strip should show.
// This is deliberately hardware-free so it can be exercised by the native
// g++ test harness (test/test_sunrise_curve.cpp) without a board.
//
// How it works:
//   1. elapsed time -> progress in permille (0..1000) of the ramp.
//   2. linear interpolation between the RGBW keyframes in Config.h.
//   3. quadratic (gamma ~2) PWM shaping per channel so equal perceptual steps
//      map to a gentle low-end ramp -- this is what keeps the dim pre-dawn
//      phase from visibly stepping on 8-bit channels (spec SS 4.7).
//
// Keyframe RGBW values are treated as PERCEPTUAL targets; the shaping step
// converts them to the PWM bytes actually written. Exact keyframes, duration,
// and the shaping exponent are tuned on hardware (Config.h placeholders).
// ============================================================================

// Perceptual->PWM shaping exponent. 2 == quadratic (spec's default feel).
// Tuned on hardware alongside the keyframes.
constexpr uint8_t SUNRISE_GAMMA = 2;

struct SunriseSample {
  Rgbw     color;     // shaped RGBW to write to every pixel
  uint16_t progress;  // 0..1000 permille of the ramp
  bool     complete;  // true once progress has reached 1000 (-> ALARM_SOUNDING)
};

namespace SunriseCurve {

// Sample the curve at `elapsedMs` after the sunrise began. Clamps to the ramp
// endpoints outside [0, durationMs]. durationMs defaults to the Config value
// but is injectable so tests can use short ramps.
SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs = SUNRISE_DURATION_MS);

} // namespace SunriseCurve
