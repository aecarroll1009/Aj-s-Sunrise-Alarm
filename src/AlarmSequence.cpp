#include "AlarmSequence.h"
#include "SunriseCurve.h"

namespace {

// The full-brightness wake color is just the end of the sunrise curve, so
// the sounding and snooze light matches where the ramp finished (spec
// 4.7/4.8).
//
// Args:
//   durationMs: the ramp's configured duration.
// Returns:
//   The color at the end of the ramp.
Rgbw fullColor(uint32_t durationMs) {
  return SunriseCurve::sample(durationMs, durationMs).color;
}

// Builds the output for a frame where the alarm is sounding: full light and
// audio on.
//
// Args:
//   sunriseDurationMs: the ramp's configured duration.
// Returns:
//   The alarm output for a sounding frame.
AlarmOutput soundingOutput(uint32_t sunriseDurationMs) {
  AlarmOutput o{true, fullColor(sunriseDurationMs), true, 0};
  return o;
}

} // namespace

AlarmSequence::AlarmSequence()
    : state_(IDLE), startMs_(0), snoozeDeadlineMs_(0) {}

void AlarmSequence::startSunrise(uint32_t nowMs) {
  state_   = SUNRISE;
  startMs_ = nowMs;
}

void AlarmSequence::resumeSounding(uint32_t nowMs) {
  (void)nowMs;
  state_ = SOUNDING;
}

AlarmOutput AlarmSequence::update(uint32_t nowMs, uint32_t sunriseDurationMs) {
  switch (state_) {
    case IDLE:
      return AlarmOutput{false, {0, 0, 0, 0}, false, 0};

    case SUNRISE: {
      SunriseSample s = SunriseCurve::sample(nowMs - startMs_, sunriseDurationMs);
      if (!s.complete) {
        return AlarmOutput{true, s.color, false, 0};
      }
      state_ = SOUNDING;
      return soundingOutput(sunriseDurationMs);
    }

    case SOUNDING:
      return soundingOutput(sunriseDurationMs);

    case SNOOZED: {
      // Wrap-safe deadline check: signed diff handles millis() rollover.
      if ((int32_t)(nowMs - snoozeDeadlineMs_) >= 0) {
        state_ = SOUNDING;
        return soundingOutput(sunriseDurationMs);
      }
      AlarmOutput o{true, fullColor(sunriseDurationMs), false, snoozeDeadlineMs_ - nowMs};
      return o;
    }
  }
  return AlarmOutput{false, {0, 0, 0, 0}, false, 0};
}

AlarmSequence::Action AlarmSequence::onSoftPower() {
  if (state_ == IDLE) return NONE;
  state_ = IDLE;
  return DISMISSED;
}

AlarmSequence::Action AlarmSequence::onEncoderLong() {
  if (state_ == IDLE) return NONE;
  state_ = IDLE;
  return CANCELLED;
}

bool AlarmSequence::onEncoderShort(uint32_t nowMs) {
  if (state_ != SOUNDING) return false;
  state_ = SNOOZED;
  snoozeDeadlineMs_ = nowMs + SNOOZE_MS;
  return true;
}

bool AlarmSequence::shouldStartSunrise(bool alarm1Flagged, bool armed,
                                       bool dismissedToday, bool active) {
  return alarm1Flagged && armed && !dismissedToday && !active;
}

bool AlarmSequence::shouldResumeOnBoot(bool armed, bool alarmTimePassedToday,
                                       bool dismissedToday) {
  return armed && alarmTimePassedToday && !dismissedToday;
}
