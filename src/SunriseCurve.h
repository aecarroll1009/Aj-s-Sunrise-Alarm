#pragma once
#include <stdint.h>
#include "Tuning.h"
#include "Types.h"

// The sunrise light ramp (spec 4.7). Pure logic.

struct SunriseSample {
  Rgbw     color;
  uint16_t progress;  // permille, 0..1000
  bool     complete;
};

namespace SunriseCurve {

// Returns the ramp color at a point in time.
//
// Args:
//   elapsedMs: time since the sunrise began; past the end clamps to the end.
//   durationMs: ramp length.
SunriseSample sample(uint32_t elapsedMs, uint32_t durationMs = SUNRISE_DURATION_MS);

}  // namespace SunriseCurve
