#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// AlarmSequence — the authoritative wake-up state machine (spec SS 4.8).
//
// PURE LOGIC: no hardware, no Arduino headers, no millis() call inside. Time is
// injected (nowMs) exactly like SunriseCurve, so the whole machine is native-
// testable (test/test_alarm_sequence.cpp).
//
// States:
//   IDLE      not in a wake sequence; the manual LightEngine owns the strip.
//   SUNRISE   ramping the fixed curve, SILENT (audio only at full brightness).
//   SOUNDING  curve done -> full light + song.
//   SNOOZED   light stays FULL, song stops, 10:00 countdown; at 0 -> SOUNDING.
//
// Control grammar (spec SS 4.8 table), honored at EVERY active stage:
//   soft power  -> DISMISSED (light+audio off, done for today)
//   enc long    -> CANCELLED (everything off)
//   enc short   -> SNOOZE, but ONLY while SOUNDING (a no-op during the ramp)
//
// The four correctness fixes live at the seams: the trigger is the DS3231
// hardware alarm flag (shouldStartSunrise), persistence keys off firedOnDate
// (dismissedToday), and audio is a level the driver reconciles via the BUSY
// pin -- this class never touches the DFPlayer or the strip directly.
// ============================================================================

struct AlarmOutput {
  bool     lightOverride;     // true -> alarm drives the strip this frame
  Rgbw     lightColor;        // color to hand LightEngine (valid if lightOverride)
  bool     audioOn;           // desired audio level; driver edges it off the BUSY pin
  uint32_t snoozeRemainingMs; // for the OLED countdown (valid while SNOOZED)
};

class AlarmSequence {
public:
  enum State  : uint8_t { IDLE, SUNRISE, SOUNDING, SNOOZED };
  enum Action : uint8_t { NONE, DISMISSED, CANCELLED };

  AlarmSequence();

  // --- entry points (called by the main loop) ------------------------------
  void startSunrise(uint32_t nowMs);    // DS3231 alarm fired -> begin the ramp
  void resumeSounding(uint32_t nowMs);  // power-loss recovery -> straight to SOUNDING

  // --- per-frame update ----------------------------------------------------
  // Advances time-based transitions and returns what to drive this frame.
  // sunriseDurationMs is injectable so tests can use short ramps.
  AlarmOutput update(uint32_t nowMs, uint32_t sunriseDurationMs = SUNRISE_DURATION_MS);

  // --- events (route these here only while active) -------------------------
  Action onSoftPower();                 // DISMISSED at any stage, else NONE
  Action onEncoderLong();               // CANCELLED at any stage, else NONE
  bool   onEncoderShort(uint32_t nowMs);// snoozes iff SOUNDING; returns true if it did

  // --- queries -------------------------------------------------------------
  State state() const  { return state_; }
  bool  active() const { return state_ != IDLE; }

  // --- pure decision helpers (no state) ------------------------------------
  // Main loop trigger (spec SS 4.9): fire only on the hardware flag, when armed,
  // not already fired/dismissed today, and not already running.
  static bool shouldStartSunrise(bool alarm1Flagged, bool armed,
                                 bool dismissedToday, bool active);

  // Boot recovery (spec SS 4.8): if the alarm time already passed today and it
  // was never dismissed, come back up SOUNDING rather than silently skipping.
  static bool shouldResumeOnBoot(bool armed, bool alarmTimePassedToday,
                                 bool dismissedToday);

private:
  State    state_;
  uint32_t startMs_;          // sunrise start, for elapsed
  uint32_t snoozeDeadlineMs_; // millis deadline (wrap-safe via signed diff)
};
