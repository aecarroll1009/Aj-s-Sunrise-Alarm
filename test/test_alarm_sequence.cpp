// Native unit test for AlarmSequence. Built and run by test/run_tests.sh.

#include <cstdio>
#include "../src/AlarmSequence.h"
#include "../src/SunriseCurve.h"

static int failures = 0;
#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

int main() {
  printf("AlarmSequence tests\n");
  const uint32_t DUR = 1000;   // 1 s test ramp

  const Rgbw FULL = SunriseCurve::sample(DUR, DUR).color;

  // The ramp is silent.
  {
    AlarmSequence a;
    CHECK(a.state() == AlarmSequence::IDLE && !a.active(), "boots IDLE");
    AlarmOutput idle = a.update(0, DUR);
    CHECK(!idle.audioOn && idle.color == (Rgbw{0, 0, 0, 0}), "IDLE drives nothing");

    a.start(1000);
    AlarmOutput mid = a.update(1000 + DUR / 2, DUR);
    CHECK(a.state() == AlarmSequence::SUNRISE, "start -> SUNRISE");
    CHECK(a.active() && !mid.audioOn, "ramp is active and silent");
  }

  // Curve completion transitions to SOUNDING + audio.
  {
    AlarmSequence a;
    a.start(0);
    AlarmOutput done = a.update(DUR, DUR);
    CHECK(a.state() == AlarmSequence::SOUNDING, "curve done -> SOUNDING");
    CHECK(done.audioOn && done.color == FULL, "SOUNDING: full light + audio on");
  }

  // Stop works from the ramp and from SOUNDING.
  {
    AlarmSequence a; a.start(0);
    a.stop();
    CHECK(a.state() == AlarmSequence::IDLE, "stop during ramp -> IDLE");
    CHECK(!a.update(500, DUR).audioOn, "after stop: no audio");

    AlarmSequence b; b.start(0); b.update(DUR, DUR);  // SOUNDING
    b.stop();
    CHECK(b.state() == AlarmSequence::IDLE, "stop while SOUNDING -> IDLE");
  }

  // Encoder short: nothing during ramp, snooze only while sounding.
  {
    AlarmSequence a; a.start(0);
    CHECK(!a.snooze(100), "enc short during ramp does nothing");
    CHECK(a.state() == AlarmSequence::SUNRISE, "still SUNRISE after ignored short-press");

    a.update(DUR, DUR);                          // -> SOUNDING
    CHECK(a.snooze(DUR), "enc short while SOUNDING snoozes");
    CHECK(a.state() == AlarmSequence::SNOOZED, "-> SNOOZED");

    AlarmOutput s = a.update(DUR + 1000, DUR);
    CHECK(a.active() && !s.audioOn && s.color == FULL, "SNOOZE: full light, audio off");
    CHECK(s.snoozeRemainingMs > 0 && s.snoozeRemainingMs <= SNOOZE_MS, "countdown within 10:00");
  }

  // Snooze expiry replays the song (back to SOUNDING).
  {
    AlarmSequence a; a.start(0); a.update(DUR, DUR);   // SOUNDING
    a.snooze(DUR);                                    // ends at DUR + SNOOZE_MS
    AlarmOutput before = a.update(DUR + SNOOZE_MS - 1, DUR);
    CHECK(a.state() == AlarmSequence::SNOOZED && !before.audioOn, "just before deadline: still snoozed");
    AlarmOutput after = a.update(DUR + SNOOZE_MS, DUR);
    CHECK(a.state() == AlarmSequence::SOUNDING && after.audioOn, "at deadline: resumes SOUNDING + audio");
  }

  // Stop works from SNOOZE too.
  {
    AlarmSequence a; a.start(0); a.update(DUR, DUR); a.snooze(DUR); // SNOOZED
    a.stop();
    CHECK(a.state() == AlarmSequence::IDLE, "stop while SNOOZED -> IDLE");
  }

  // millis() wraparound on the snooze deadline.
  {
    AlarmSequence a; a.start(0); a.update(DUR, DUR);   // SOUNDING
    uint32_t nearMax = 0xFFFFFFFFUL - 100000UL;
    a.snooze(nearMax);                                // deadline wraps past 0
    CHECK(a.state() == AlarmSequence::SNOOZED, "snoozed near uint32 max");
    CHECK(a.update(400000, DUR).snoozeRemainingMs > 0 &&
          a.state() == AlarmSequence::SNOOZED, "still snoozed across the wrap");
    a.update(600000, DUR);
    CHECK(a.state() == AlarmSequence::SOUNDING, "resumes after wrapped deadline");
  }

  // Pure trigger helper (spec 4.9 truth table).
  CHECK( AlarmSequence::shouldStart(true,  true,  false, false), "flag+armed+fresh+idle -> fire");
  CHECK(!AlarmSequence::shouldStart(false, true,  false, false), "no flag -> no fire");
  CHECK(!AlarmSequence::shouldStart(true,  false, false, false), "disarmed -> no fire");
  CHECK(!AlarmSequence::shouldStart(true,  true,  true,  false), "already dismissed today -> no fire");
  CHECK(!AlarmSequence::shouldStart(true,  true,  false, true),  "already running -> no re-fire");

  // Pure boot-recovery helper (spec 4.8).
  CHECK( AlarmSequence::shouldResumeOnBoot(true,  true,  false), "passed today + not dismissed -> resume");
  CHECK(!AlarmSequence::shouldResumeOnBoot(true,  true,  true),  "was dismissed -> stay idle");
  CHECK(!AlarmSequence::shouldResumeOnBoot(true,  false, false), "time not reached -> stay idle");
  CHECK(!AlarmSequence::shouldResumeOnBoot(false, true,  false), "disarmed -> stay idle");

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
