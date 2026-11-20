#include "StripDriver.h"

namespace {
constexpr uint16_t MIN_WRITE_INTERVAL_MS = 30;
}  // namespace

void StripDriver::begin() {
  strip_.begin();
  strip_.clear();
  strip_.show();
  shown_ = Rgbw{0, 0, 0, 0};
}

void StripDriver::show(Rgbw c, uint32_t nowMs) {
  if (c == shown_) return;
  if (nowMs - lastWriteMs_ < MIN_WRITE_INTERVAL_MS) return;  // retry next frame

  strip_.fill(strip_.Color(c.r, c.g, c.b, c.w));
  strip_.show();
  shown_ = c;
  lastWriteMs_ = nowMs;
}
