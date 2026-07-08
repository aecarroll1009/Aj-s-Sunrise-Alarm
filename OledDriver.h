#pragma once
#include <U8g2lib.h>
#include "Config.h"
#include "UIMenu.h"

// ============================================================================
// OledDriver — 128x64 display (spec SS 4.4, 4.10, 4, fix 3).
//
// U8g2 in PAGE-BUFFER mode (the "_1_" constructor, ~128 B) -- never a full 1 KB
// framebuffer, which wouldn't safely fit in the ATmega328P's 2 KB SRAM. Renders
// the view-model that UIMenu exposes. On HOME the panel auto-dims after ~10-15 s
// idle (it is a real light source in a dark bedroom) and wakes on any input.
//
// Pixel positions and fonts here are a reasonable first cut; final layout is
// tuned on the actual panel at bring-up.
// ============================================================================

class OledDriver {
public:
  void begin();

  // Draw one frame. `lampName` is the current preset name (HOME "Lamp:" line).
  // Alarm fields drive a dedicated overlay while the wake sequence is active.
  void render(const UIMenu& ui, bool lampOn, const char* lampName,
              bool alarmActive, bool snoozing, uint32_t snoozeRemainingMs,
              uint32_t nowMs, bool activity);

private:
#if OLED_IS_SH1106
  U8G2_SH1106_128X64_NONAME_1_HW_I2C  u8g2_{U8G2_R0, U8X8_PIN_NONE};
#else
  U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};
#endif
  uint32_t lastActivityMs_{0};
  bool     dimmed_{false};

  void drawHome(const UIMenu& ui, bool lampOn, const char* lampName);
  void drawMenu(const UIMenu& ui);
  void drawSetAlarm(const UIMenu& ui, uint32_t nowMs);
  void drawSetTime(const UIMenu& ui, uint32_t nowMs);
  void drawColorPreset(const UIMenu& ui);
  void drawAlarm(bool snoozing, uint32_t snoozeRemainingMs);
};
