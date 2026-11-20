// Native unit test for SunriseCurve. Built and run by test/run_tests.sh.

#include <cstdio>
#include <cstdlib>
#include "../src/SunriseCurve.h"

static int failures = 0;

#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

static uint16_t luma(const Rgbw& c) {
  // Rough brightness, weighting W since it dominates late in the ramp.
  return (uint16_t)c.r + c.g + c.b + 2 * (uint16_t)c.w;
}

int main() {
  const uint32_t D = 25UL * 60UL * 1000UL; // 25 min test ramp

  printf("SunriseCurve tests (duration=%lu ms)\n", (unsigned long)D);

  // Endpoints.
  SunriseSample start = SunriseCurve::sample(0, D);
  CHECK(start.progress == 0, "t=0 -> progress 0");
  CHECK(!start.complete, "t=0 -> not complete");
  CHECK(start.color.r >= 1, "t=0 -> red starts at PWM>=1 (no black pop)");

  SunriseSample end = SunriseCurve::sample(D, D);
  CHECK(end.progress == 1000, "t=D -> progress 1000");
  CHECK(end.complete, "t=D -> complete (alarm starts SOUNDING)");
  CHECK(end.color.w >= end.color.r, "t=D -> W dominant at full day");

  // Clamping.
  SunriseSample over = SunriseCurve::sample(3 * D, D);
  CHECK(over.progress == 1000 && over.complete, "t>D clamps to complete");
  CHECK(over.color.w == end.color.w && over.color.r == end.color.r,
        "t>D holds the final color");

  SunriseSample zeroDur = SunriseCurve::sample(0, 0);
  CHECK(zeroDur.complete, "duration 0 -> immediately complete (no div-by-zero)");

  // Midpoint is between the endpoints, exclusive of them.
  SunriseSample mid = SunriseCurve::sample(D / 2, D);
  CHECK(mid.progress > 480 && mid.progress < 520, "t=D/2 -> progress ~500");
  CHECK(!mid.complete, "t=D/2 -> not complete");

  // Brightness is non-decreasing across the whole ramp.
  bool mono = true;
  uint16_t prev = 0;
  int firstNonZeroPct = -1;
  for (int pct = 0; pct <= 100; ++pct) {
    uint32_t t = (uint32_t)((uint64_t)D * pct / 100);
    SunriseSample s = SunriseCurve::sample(t, D);
    uint16_t l = luma(s.color);
    if (l + 3 < prev) {  // rounding tolerance
      mono = false;
      printf("  (non-monotonic at %d%%: luma %u < prev %u)\n", pct, l, prev);
    }
    prev = l;
    if (firstNonZeroPct < 0 && (s.color.r || s.color.g || s.color.b || s.color.w))
      firstNonZeroPct = pct;
  }
  CHECK(mono, "brightness (luma proxy) is non-decreasing across the ramp");
  CHECK(firstNonZeroPct == 0, "light is on from t=0 (starts dim, never fully dark)");

  // Mid-ramp is never fully off.
  bool bytesOk = true;
  for (int pct = 0; pct <= 100; ++pct) {
    uint32_t t = (uint32_t)((uint64_t)D * pct / 100);
    SunriseSample s = SunriseCurve::sample(t, D);
    if (s.progress > 0 && s.progress < 1000 && s.color.r == 0 && s.color.g == 0 &&
        s.color.b == 0 && s.color.w == 0)
      bytesOk = false;
  }
  CHECK(bytesOk, "mid-ramp is never fully off");

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
