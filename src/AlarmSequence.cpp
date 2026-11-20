#include "AlarmSequence.h"
#include "SunriseCurve.h"

void AlarmSequence::start(uint32_t nowMs) {
  state_   = SUNRISE;
  startMs_ = nowMs;
}

AlarmOutput AlarmSequence::update(uint32_t nowMs, uint32_t durationMs) {
  if (state_ == IDLE) return AlarmOutput{Rgbw{0, 0, 0, 0}, false, 0};

  if (state_ == SUNRISE) {
    SunriseSample s = SunriseCurve::sample(nowMs - startMs_, durationMs);
    if (!s.complete) return AlarmOutput{s.color, false, 0};
    state_ = SOUNDING;
  }

  // Signed difference survives millis() rollover.
  if (state_ == SNOOZED && (int32_t)(nowMs - snoozeEndMs_) >= 0) state_ = SOUNDING;

  const Rgbw full = SunriseCurve::sample(durationMs, durationMs).color;
  if (state_ == SNOOZED) return AlarmOutput{full, false, snoozeEndMs_ - nowMs};
  return AlarmOutput{full, true, 0};
}

bool AlarmSequence::snooze(uint32_t nowMs) {
  if (state_ != SOUNDING) return false;
  state_ = SNOOZED;
  snoozeEndMs_ = nowMs + SNOOZE_MS;
  return true;
}
