#pragma once
#include <stdint.h>
#include "Config.h"

// The fixed wake-up light ramp (spec 4.7).
//
// Pure logic, no hardware calls or Arduino headers, so it can run under the
// native g++ test harness in test/test_sunrise_curve.cpp. Given the elapsed
// time since the sunrise started, sample() returns the RGBW color the strip
// should show.
//
// The ramp works in two steps: linear interpolation between the RGBW
// keyframes in Config.h, then a gamma shaping pass so equal perceptual steps
// look even on 8-bit PWM channels instead of stepping visibly near black.
// Keyframe values are perceptual targets; shaping converts them to the PWM
// bytes actually written. Exact keyframes, duration, and the shaping
// exponent are tuned on hardware (Config.h placeholders).

// Perceptual-to-PWM shaping exponent. 2 means quadratic, the spec's default
// feel. Tuned on hardware alongside the keyframes.
constexpr uint8_t SUNRISE_GAMMA = 2;

struct SunriseSample {
  Rgbw     color;     // shaped RGBW to write to every pixel
  uint16_t progress;  // 0..1000 permille of the ramp
  bool     complete;  // true once progress has reached 1000 (triggers ALARM_SOUNDING)
};

namespace SunriseCurve {

// Samples the ramp at a point in time.
//
// Args:
//   elapsedMs: milliseconds since the sunrise began.
//   durationMs: total ramp length. Defaults to the configured duration; tests
//       inject shorter values. Elapsed times outside [0, durationMs] clamp to
//       the nearest endpoint.
// Returns:
//   The shaped color to display, the ramp's progress in permille, and
//   whether it has reached the end.
SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs = SUNRISE_DURATION_MS);

} // namespace SunriseCurve
