#pragma once
#include <U8g2lib.h>
#include "Config.h"
#include "UIMenu.h"

// 128x64 display (spec 4.4, 4.10, fix 3).
//
// Uses U8g2 in page-buffer mode (the "_1_" constructor, about 128 bytes),
// never a full 1 KB framebuffer, which wouldn't safely fit in the
// ATmega328P's 2 KB of SRAM. Renders the view-model that UIMenu exposes. On
// HOME the panel auto-dims after about 10-15 seconds idle, since it's a real
// light source in a dark bedroom, and wakes on any input.
//
// Pixel positions and fonts here are a reasonable first cut; the final
// layout is tuned on the actual panel at bring-up.
class OledDriver {
public:
  void begin();

  // Draws one frame.
  //
  // Args:
  //   ui: current menu view-model.
  //   lampOn: whether the manual lamp is on.
  //   lampName: current preset name, shown on the HOME "Lamp:" line.
  //   alarmActive: whether the wake sequence is running; when true, this
  //       draws the alarm overlay instead of the normal screen.
  //   snoozing: whether the alarm is currently snoozed.
  //   snoozeRemainingMs: time left in the snooze countdown, if snoozing.
  //   nowMs: current time, used for the edit-field blink and idle timeout.
  //   activity: whether any input happened this frame, which resets the
  //       idle timer and wakes the panel.
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

  // Updates the idle-dim state machine and applies any resulting power-save
  // change (spec 4.10: dimming applies only on HOME; a menu or an active
  // alarm stays awake).
  //
  // Args:
  //   activity: whether any input happened this frame.
  //   alarmActive: whether the wake sequence is running.
  //   screen: the screen currently shown.
  //   nowMs: current time.
  // Returns:
  //   True if the panel is now powered down and this frame's draw should be
  //   skipped.
  bool updatePowerSave(bool activity, bool alarmActive, UIMenu::Screen screen,
                       uint32_t nowMs);

  void drawHome(const UIMenu& ui, bool lampOn, const char* lampName);
  void drawMenu(const UIMenu& ui);
  void drawSetAlarm(const UIMenu& ui, uint32_t nowMs);
  void drawSetTime(const UIMenu& ui, uint32_t nowMs);
  void drawColorPreset(const UIMenu& ui);
  void drawAlarm(bool snoozing, uint32_t snoozeRemainingMs);
};
