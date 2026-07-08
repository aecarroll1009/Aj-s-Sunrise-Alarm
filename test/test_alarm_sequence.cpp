// Native (host g++) unit test for AlarmSequence — runs without any hardware.
//
//   g++ -std=c++11 -I. -Wall -Wextra AlarmSequence.cpp SunriseCurve.cpp test/test_alarm_sequence.cpp -o test/run_alarm && test/run_alarm

#include <cstdio>
#include "../AlarmSequence.h"

static int failures = 0;
#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

int main() {
  printf("AlarmSequence tests\n");
  const uint32_t DUR = 1000;   // 1 s test ramp

  // --- ramp is silent, overrides the light ------------------------------
  {
    AlarmSequence a;
    CHECK(a.state() == AlarmSequence::IDLE && !a.active(), "boots IDLE");
    AlarmOutput idle = a.update(0, DUR);
    CHECK(!idle.lightOverride && !idle.audioOn, "IDLE drives nothing");

    a.startSunrise(1000);
    AlarmOutput mid = a.update(1000 + DUR / 2, DUR);
    CHECK(a.state() == AlarmSequence::SUNRISE, "startSunrise -> SUNRISE");
    CHECK(mid.lightOverride && !mid.audioOn, "ramp overrides light, stays silent");
  }

  // --- curve completion transitions to SOUNDING + audio -----------------
  {
    AlarmSequence a;
    a.startSunrise(0);
    AlarmOutput done = a.update(DUR, DUR);       // elapsed == duration -> complete
    CHECK(a.state() == AlarmSequence::SOUNDING, "curve done -> SOUNDING");
    CHECK(done.lightOverride && done.audioOn, "SOUNDING: full light + audio on");
  }

  // --- soft power dismisses at any stage --------------------------------
  {
    AlarmSequence a; a.startSunrise(0);
    CHECK(a.onSoftPower() == AlarmSequence::DISMISSED, "soft power in SUNRISE -> DISMISSED");
    CHECK(a.state() == AlarmSequence::IDLE, "dismiss -> IDLE");
    CHECK(!a.update(500, DUR).lightOverride, "after dismiss the light is released");
    CHECK(a.onSoftPower() == AlarmSequence::NONE, "soft power while IDLE -> NONE");
  }

  // --- encoder long cancels at any stage --------------------------------
  {
    AlarmSequence a; a.startSunrise(0); a.update(DUR, DUR); // SOUNDING
    CHECK(a.onEncoderLong() == AlarmSequence::CANCELLED, "enc long in SOUNDING -> CANCELLED");
    CHECK(a.state() == AlarmSequence::IDLE, "cancel -> IDLE");
  }

  // --- encoder short: nothing during ramp, snooze only while sounding ---
  {
    AlarmSequence a; a.startSunrise(0);
    CHECK(!a.onEncoderShort(100), "enc short during ramp does nothing");
    CHECK(a.state() == AlarmSequence::SUNRISE, "still SUNRISE after ignored short-press");

    a.update(DUR, DUR);                          // -> SOUNDING
    CHECK(a.onEncoderShort(DUR), "enc short while SOUNDING snoozes");
    CHECK(a.state() == AlarmSequence::SNOOZED, "-> SNOOZED");

    AlarmOutput s = a.update(DUR + 1000, DUR);
    CHECK(s.lightOverride && !s.audioOn, "SNOOZE: light FULL, audio off");
    CHECK(s.snoozeRemainingMs > 0 && s.snoozeRemainingMs <= SNOOZE_MS, "countdown within 10:00");
  }

  // --- snooze expiry replays the song (back to SOUNDING) ----------------
  {
    AlarmSequence a; a.startSunrise(0); a.update(DUR, DUR);   // SOUNDING
    a.onEncoderShort(DUR);                                    // SNOOZED, deadline = DUR + SNOOZE_MS
    AlarmOutput before = a.update(DUR + SNOOZE_MS - 1, DUR);
    CHECK(a.state() == AlarmSequence::SNOOZED && !before.audioOn, "just before deadline: still snoozed");
    AlarmOutput after = a.update(DUR + SNOOZE_MS, DUR);
    CHECK(a.state() == AlarmSequence::SOUNDING && after.audioOn, "at deadline: resumes SOUNDING + audio");
  }

  // --- dismiss / cancel work from SNOOZE too ----------------------------
  {
    AlarmSequence a; a.startSunrise(0); a.update(DUR, DUR); a.onEncoderShort(DUR); // SNOOZED
    CHECK(a.onSoftPower() == AlarmSequence::DISMISSED, "soft power in SNOOZE -> DISMISSED");
    AlarmSequence b; b.startSunrise(0); b.update(DUR, DUR); b.onEncoderShort(DUR);
    CHECK(b.onEncoderLong() == AlarmSequence::CANCELLED, "enc long in SNOOZE -> CANCELLED");
  }

  // --- millis() wraparound on the snooze deadline -----------------------
  {
    AlarmSequence a; a.startSunrise(0); a.update(DUR, DUR);   // SOUNDING
    uint32_t nearMax = 0xFFFFFFFFUL - 100000UL;               // deadline wraps past 0
    a.onEncoderShort(nearMax);                                // deadline = nearMax + 600000 (wrapped)
    CHECK(a.state() == AlarmSequence::SNOOZED, "snoozed near uint32 max");
    CHECK(a.update(400000, DUR).snoozeRemainingMs > 0 &&
          a.state() == AlarmSequence::SNOOZED, "still snoozed across the wrap");
    a.update(600000, DUR);
    CHECK(a.state() == AlarmSequence::SOUNDING, "resumes after wrapped deadline");
  }

  // --- pure trigger helper (spec SS 4.9 truth table) --------------------
  CHECK( AlarmSequence::shouldStartSunrise(true,  true,  false, false), "flag+armed+fresh+idle -> fire");
  CHECK(!AlarmSequence::shouldStartSunrise(false, true,  false, false), "no flag -> no fire");
  CHECK(!AlarmSequence::shouldStartSunrise(true,  false, false, false), "disarmed -> no fire");
  CHECK(!AlarmSequence::shouldStartSunrise(true,  true,  true,  false), "already dismissed today -> no fire");
  CHECK(!AlarmSequence::shouldStartSunrise(true,  true,  false, true),  "already running -> no re-fire");

  // --- pure boot-recovery helper (spec SS 4.8) --------------------------
  CHECK( AlarmSequence::shouldResumeOnBoot(true,  true,  false), "passed today + not dismissed -> resume");
  CHECK(!AlarmSequence::shouldResumeOnBoot(true,  true,  true),  "was dismissed -> stay idle");
  CHECK(!AlarmSequence::shouldResumeOnBoot(true,  false, false), "time not reached -> stay idle");
  CHECK(!AlarmSequence::shouldResumeOnBoot(false, true,  false), "disarmed -> stay idle");

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
