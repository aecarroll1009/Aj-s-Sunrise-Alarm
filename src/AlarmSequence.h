#pragma once
#include <stdint.h>
#include "Tuning.h"
#include "Types.h"

// The wake-up sequence (spec 4.8). Pure logic.
//
// IDLE -> SUNRISE (silent ramp) -> SOUNDING (full light and song).
// SNOOZED keeps the light on with the song off, then returns to SOUNDING.

// What the alarm wants this frame.
struct AlarmOutput {
  Rgbw     color;
  bool     audioOn;
  uint32_t snoozeRemainingMs;  // while SNOOZED
};

class AlarmSequence {
 public:
  enum State : uint8_t { IDLE, SUNRISE, SOUNDING, SNOOZED };

  // Starts the sunrise ramp at nowMs.
  void start(uint32_t nowMs);

  // Skips the ramp; used after a power cut during an alarm.
  void resumeSounding() { state_ = SOUNDING; }

  // Advances the sequence and returns this frame's output.
  //
  // Args:
  //   nowMs: current time.
  //   durationMs: ramp length.
  AlarmOutput update(uint32_t nowMs, uint32_t durationMs = SUNRISE_DURATION_MS);

  // Ends the alarm from any state.
  void stop() { state_ = IDLE; }

  // Snoozes until nowMs + SNOOZE_MS.
  //
  // Returns:
  //   True if it snoozed; only possible while SOUNDING.
  bool snooze(uint32_t nowMs);

  State state() const  { return state_; }
  bool  active() const { return state_ != IDLE; }

  // Returns whether to start the sunrise (spec 4.9).
  //
  // Args:
  //   rtcFlag: the DS3231 Alarm1 flag.
  //   armed: whether the alarm is armed.
  //   doneToday: whether it already ran and was stopped today.
  //   active: whether it's already running.
  static bool shouldStart(bool rtcFlag, bool armed, bool doneToday, bool active) {
    return rtcFlag && armed && !doneToday && !active;
  }

  // Returns whether boot should resume SOUNDING because power was cut
  // during today's alarm (spec 4.8).
  static bool shouldResumeOnBoot(bool armed, bool timePassedToday, bool doneToday) {
    return armed && timePassedToday && !doneToday;
  }

 private:
  State    state_ = IDLE;
  uint32_t startMs_ = 0;
  uint32_t snoozeEndMs_ = 0;
};
