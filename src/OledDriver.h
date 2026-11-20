#pragma once
#include <U8g2lib.h>
#include "Pins.h"
#include "Screens.h"
#include "UIMenu.h"

// The 128x64 OLED (spec 4.10).
//
// Draws page by page (about 128 bytes of RAM) since a full 1 KB framebuffer
// won't fit in the Nano's 2 KB. Sleeps after a few seconds idle on HOME.
class OledDriver {
 public:
  void begin();

  // Draws one frame, unless the panel is asleep.
  //
  // Args:
  //   activity: whether there was input this frame; wakes the panel.
  void render(const UIMenu& ui, const DisplayStatus& status, uint32_t nowMs,
              bool activity);

 private:
  // Sleeps or wakes the panel.
  //
  // Returns:
  //   True if the panel is asleep.
  bool updateSleep(bool canSleep, uint32_t nowMs, bool activity);

#if OLED_DRIVER == OLED_SSD1309
  U8G2_SSD1309_128X64_NONAME2_1_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};
#elif OLED_DRIVER == OLED_SH1106
  U8G2_SH1106_128X64_NONAME_1_HW_I2C   u8g2_{U8G2_R0, U8X8_PIN_NONE};
#else
  U8G2_SSD1306_128X64_NONAME_1_HW_I2C  u8g2_{U8G2_R0, U8X8_PIN_NONE};
#endif
  uint32_t lastActivityMs_ = 0;
  bool     asleep_ = false;
};
