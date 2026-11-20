#pragma once
#include <stdint.h>

// Value types shared across modules.

// Pixel color, each channel 0..255.
struct Rgbw { uint8_t r, g, b, w; };

inline bool operator==(const Rgbw& a, const Rgbw& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
}
inline bool operator!=(const Rgbw& a, const Rgbw& b) { return !(a == b); }

// Time of day on a 12-hour clock; hour12 is 1..12.
struct ClockHM { uint8_t hour12; uint8_t minute; bool pm; };

struct CalDate { uint8_t month; uint8_t day; uint16_t year; };
