#include "AlarmSequence.h"
#include "SunriseCurve.h"

namespace {
// The "full brightness" wake color is just the end of the sunrise curve, so the
// sounding/snooze light matches where the ramp finished (spec SS 4.7/4.8).
Rgbw fullColor(uint32_t durationMs) {
  return SunriseCurve::sample(durationMs, durationMs).color;
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
  AlarmOutput o{false, {0, 0, 0, 0}, false, 0};

  switch (state_) {
    case IDLE:
      break;

    case SUNRISE: {
      SunriseSample s = SunriseCurve::sample(nowMs - startMs_, sunriseDurationMs);
      if (s.complete) {
        state_ = SOUNDING;                 // curve done -> full light + song
        o.lightOverride = true;
        o.lightColor    = fullColor(sunriseDurationMs);
        o.audioOn       = true;
      } else {
        o.lightOverride = true;
        o.lightColor    = s.color;         // ramping, silent
      }
      break;
    }

    case SOUNDING:
      o.lightOverride = true;
      o.lightColor    = fullColor(sunriseDurationMs);
      o.audioOn       = true;
      break;

    case SNOOZED:
      // Wrap-safe deadline check: signed diff handles millis() rollover.
      if ((int32_t)(nowMs - snoozeDeadlineMs_) >= 0) {
        state_ = SOUNDING;                 // countdown hit 0 -> replay song
        o.lightOverride = true;
        o.lightColor    = fullColor(sunriseDurationMs);
        o.audioOn       = true;
      } else {
        o.lightOverride     = true;
        o.lightColor        = fullColor(sunriseDurationMs); // light stays FULL
        o.audioOn           = false;                        // song stopped
        o.snoozeRemainingMs = snoozeDeadlineMs_ - nowMs;
      }
      break;
  }
  return o;
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
  if (state_ != SOUNDING) return false;    // snooze is only offered while sounding
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
