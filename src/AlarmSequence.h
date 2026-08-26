#pragma once
#include <stdint.h>
#include "Config.h"

// The authoritative wake-up state machine (spec 4.8).
//
// Pure logic: no hardware, no Arduino headers, no millis() call inside. Time
// is injected (nowMs) exactly like SunriseCurve, so the whole machine is
// testable natively in test/test_alarm_sequence.cpp.
//
// States:
//   IDLE      not in a wake sequence; the manual LightEngine owns the strip.
//   SUNRISE   ramping the fixed curve, silent until it reaches full
//             brightness.
//   SOUNDING  curve done: full light and song.
//   SNOOZED   light stays at full brightness, song stops, a 10:00 countdown
//             runs; at zero it returns to SOUNDING.
//
// Control grammar (spec 4.8 table), honored at every active stage:
//   soft power  -> DISMISSED (light and audio off, done for today)
//   enc long    -> CANCELLED (everything off)
//   enc short   -> SNOOZE, but only while SOUNDING (a no-op during the ramp)
//
// Three things keep this correct at the seams: the trigger is the DS3231
// hardware alarm flag (shouldStartSunrise), persistence keys off
// firedOnDate (dismissedToday), and audio is a level the driver reconciles
// via the BUSY pin. This class never touches the DFPlayer or the strip
// directly.

struct AlarmOutput {
  bool     lightOverride;     // true if the alarm drives the strip this frame
  Rgbw     lightColor;        // color to hand LightEngine (valid if lightOverride)
  bool     audioOn;           // desired audio level; driver edges it off the BUSY pin
  uint32_t snoozeRemainingMs; // for the OLED countdown (valid while SNOOZED)
};

class AlarmSequence {
public:
  enum State  : uint8_t { IDLE, SUNRISE, SOUNDING, SNOOZED };
  enum Action : uint8_t { NONE, DISMISSED, CANCELLED };

  // Constructs an alarm that boots idle.
  AlarmSequence();

  // Begins the wake sequence because the DS3231 alarm fired.
  //
  // Args:
  //   nowMs: current time, used as the ramp's start time.
  void startSunrise(uint32_t nowMs);

  // Recovers from a power loss straight into SOUNDING, skipping the ramp.
  //
  // Args:
  //   nowMs: current time (unused; kept for symmetry with startSunrise).
  void resumeSounding(uint32_t nowMs);

  // Advances time-based transitions and reports what to drive this frame.
  //
  // Args:
  //   nowMs: current time.
  //   sunriseDurationMs: ramp length; defaults to the configured duration,
  //       injectable so tests can use short ramps.
  // Returns:
  //   The light and audio output for this frame.
  AlarmOutput update(uint32_t nowMs, uint32_t sunriseDurationMs = SUNRISE_DURATION_MS);

  // Handles the soft-power button. Route this only while the alarm is
  // active; the caller is responsible for checking that.
  //
  // Returns:
  //   DISMISSED at any active stage, NONE if already idle.
  Action onSoftPower();

  // Handles a long encoder press. Route this only while the alarm is active.
  //
  // Returns:
  //   CANCELLED at any active stage, NONE if already idle.
  Action onEncoderLong();

  // Handles a short encoder press. Route this only while the alarm is
  // active.
  //
  // Args:
  //   nowMs: current time, used to set the snooze deadline.
  // Returns:
  //   True if this snoozed the alarm, which only happens while SOUNDING.
  bool onEncoderShort(uint32_t nowMs);

  State state() const  { return state_; }
  bool  active() const { return state_ != IDLE; }

  // Decides whether the main loop should start the sunrise (spec 4.9).
  //
  // Args:
  //   alarm1Flagged: the DS3231 hardware alarm flag.
  //   armed: whether the alarm is currently armed.
  //   dismissedToday: whether it already fired and was dismissed today.
  //   active: whether the sequence is already running.
  // Returns:
  //   True only when flagged and armed, and not already fired or running.
  static bool shouldStartSunrise(bool alarm1Flagged, bool armed,
                                 bool dismissedToday, bool active);

  // Decides whether boot recovery should resume SOUNDING (spec 4.8): if the
  // alarm time already passed today and it was never dismissed, come back
  // up sounding rather than silently skipping it.
  //
  // Args:
  //   armed: whether the alarm is currently armed.
  //   alarmTimePassedToday: whether the alarm time has already passed today.
  //   dismissedToday: whether it was already dismissed today.
  // Returns:
  //   True if boot should resume SOUNDING.
  static bool shouldResumeOnBoot(bool armed, bool alarmTimePassedToday,
                                 bool dismissedToday);

private:
  State    state_;
  uint32_t startMs_;          // sunrise start, for elapsed time
  uint32_t snoozeDeadlineMs_; // millis deadline (wrap-safe via signed diff)
};
