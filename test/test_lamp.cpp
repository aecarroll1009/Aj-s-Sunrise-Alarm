// Native unit test for Lamp. Built and run by test/run_tests.sh.

#include <cstdio>
#include "../src/Lamp.h"

static int failures = 0;
#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

int main() {
  printf("Lamp tests\n");
  const Rgbw BLACK{0, 0, 0, 0};

  // Power.
  Lamp lamp;
  CHECK(!lamp.isOn(), "boots off");
  CHECK(lamp.color() == BLACK, "off shows black");
  lamp.togglePower();
  CHECK(lamp.isOn(), "toggle turns it on");
  lamp.togglePower();
  CHECK(!lamp.isOn(), "toggle again turns it off");
  lamp.togglePower();
  lamp.turnOff();
  CHECK(!lamp.isOn(), "turnOff turns it off");

  // Color is the preset scaled by brightness.
  lamp.togglePower();
  lamp.setPreset(0);
  lamp.setBrightness(255);
  CHECK(lamp.color() == COLOR_PRESETS[0].color, "full brightness shows the preset color");

  lamp.setBrightness(128);
  Rgbw half = lamp.color();
  Rgbw base = COLOR_PRESETS[0].color;
  CHECK(half.r <= base.r && half.w <= base.w &&
        (base.r == 0 || half.r < base.r), "half brightness dims the channels");

  // Brightness limits.
  lamp.setBrightness(0);
  CHECK(lamp.brightness() == MIN_BRIGHTNESS, "brightness never goes below the minimum");
  lamp.adjustBrightness(1000);
  CHECK(lamp.brightness() == 255, "brightness tops out at 255");
  lamp.adjustBrightness(-1000);
  CHECK(lamp.brightness() == MIN_BRIGHTNESS, "turning down stops at the minimum");

  // Brightness survives power toggles.
  lamp.setBrightness(77);
  lamp.togglePower();
  lamp.togglePower();
  CHECK(lamp.brightness() == 77, "brightness remembered across off/on");

  // Preset index wraps.
  lamp.setPreset(COLOR_PRESET_COUNT + 2);
  CHECK(lamp.preset() == 2, "out-of-range preset wraps");

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
