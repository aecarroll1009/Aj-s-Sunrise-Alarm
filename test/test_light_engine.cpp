// Native (host g++) unit test for LightEngine — runs without any hardware.
//
//   g++ -std=c++11 -I. -Wall -Wextra LightEngine.cpp test/test_light_engine.cpp -o test/run_light && test/run_light

#include <cstdio>
#include "../LightEngine.h"

static int failures = 0;
#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

static bool eq(const Rgbw& a, const Rgbw& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
}

int main() {
  printf("LightEngine tests\n");

  // --- defaults ----------------------------------------------------------
  LightEngine le;
  CHECK(le.state() == LightEngine::LIGHT_OFF, "boots LIGHT_OFF");
  CHECK(!le.isOn(), "boots not-on");
  CHECK(eq(le.outputColor(), Rgbw{0,0,0,0}), "OFF outputs black");

  // --- soft power toggles OFF <-> MANUAL ---------------------------------
  le.togglePower();
  CHECK(le.state() == LightEngine::MANUAL, "power -> MANUAL");
  CHECK(le.isOn(), "MANUAL is on");
  le.togglePower();
  CHECK(le.state() == LightEngine::LIGHT_OFF, "power again -> OFF");

  // --- MANUAL outputs preset scaled by brightness ------------------------
  le.togglePower();                     // MANUAL
  le.setPreset(0);                      // COLOR_PRESETS[0]
  le.setBrightness(255);
  CHECK(eq(le.outputColor(), COLOR_PRESETS[0].c), "full brightness == preset color");

  le.setBrightness(128);
  Rgbw half = le.outputColor();
  Rgbw base = COLOR_PRESETS[0].c;
  CHECK(half.r <= base.r && half.w <= base.w &&
        (base.r == 0 || half.r < base.r), "half brightness dims channels");

  // --- brightness clamps -------------------------------------------------
  le.setBrightness(0);
  CHECK(le.brightness() == MANUAL_BRIGHTNESS_MIN, "brightness floors at MIN (stays lit)");
  le.adjustBrightness(1000);
  CHECK(le.brightness() == 255, "brightness ceils at 255");
  le.adjustBrightness(-1000);
  CHECK(le.brightness() == MANUAL_BRIGHTNESS_MIN, "rotate-down floors, never 0");

  // --- remembered brightness across power cycle --------------------------
  le.setBrightness(77);
  le.togglePower();                     // OFF
  le.togglePower();                     // MANUAL
  CHECK(le.brightness() == 77, "brightness remembered across off/on");

  // --- preset scroll wraps both directions -------------------------------
  le.setPreset(0);
  le.nextPreset(-1);
  CHECK(le.presetIndex() == COLOR_PRESET_COUNT - 1, "preset scroll wraps down");
  le.nextPreset(1);
  CHECK(le.presetIndex() == 0, "preset scroll wraps up");
  le.setPreset(COLOR_PRESET_COUNT + 2);
  CHECK(le.presetIndex() == 2, "setPreset wraps out-of-range index");

  // --- alarm override is authoritative -----------------------------------
  le.togglePower();                     // -> OFF (was MANUAL)
  le.beginOverride();
  CHECK(le.state() == LightEngine::SUNRISE, "beginOverride -> SUNRISE");
  Rgbw sun{200, 90, 10, 120};
  le.setOverrideColor(sun);
  CHECK(eq(le.outputColor(), sun), "override color drives output, ignoring manual");
  CHECK(le.isOn(), "SUNRISE counts as on");
  le.togglePower();                     // must be ignored during override
  CHECK(le.state() == LightEngine::SUNRISE, "soft power ignored during override");

  // --- dismiss/cancel ends the override into OFF -------------------------
  le.endOverrideOff();
  CHECK(le.state() == LightEngine::LIGHT_OFF, "endOverrideOff -> OFF");
  CHECK(eq(le.outputColor(), Rgbw{0,0,0,0}), "OFF after dismiss outputs black");

  // --- restore from EEPROM -----------------------------------------------
  LightEngine le2;
  le2.restore(3, 150);
  CHECK(le2.presetIndex() == (3 % COLOR_PRESET_COUNT), "restore sets preset");
  CHECK(le2.brightness() == 150, "restore sets brightness");

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
