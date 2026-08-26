#include "OledDriver.h"
#include <Arduino.h>
#include <stdio.h>

// Fonts kept in one place so bring-up tuning is a one-line change.
#define FONT_SMALL u8g2_font_6x12_tf       // menus, labels, hints
#define FONT_BIG   u8g2_font_logisoso24_tn // big HOME clock (digits + ':')

namespace {

// 3-character names packed in flash; index = value*3. Saves RAM versus a
// const char*[] table.
const char MONS[] PROGMEM = "---JanFebMarAprMayJunJulAugSepOctNovDec";
const char DOWS[] PROGMEM = "SunMonTueWedThuFriSat";

// Copies one 3-character name out of a flash-resident table.
//
// Args:
//   table: flash table of packed 3-character names.
//   idx: entry index into the table.
//   out: buffer of at least 4 bytes to receive the name and a null terminator.
void name3(const char* table, uint8_t idx, char out[4]) {
  memcpy_P(out, table + (uint16_t)idx * 3, 3);
  out[3] = '\0';
}

// Computes the day of week using Sakamoto's algorithm, since the RTC only
// gives year, month, and day.
//
// Args:
//   y: calendar year.
//   m: calendar month, 1..12.
//   d: calendar day, 1..31.
// Returns:
//   Day of week, 0 for Sunday.
uint8_t dow(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}

// Returns whether an editing field should be in its visible blink phase.
bool blinkOn(uint32_t nowMs) { return ((nowMs / EDIT_FLASH_MS) & 1) == 0; }

} // namespace

// Draws a flash-resident (PSTR) label without keeping a permanent RAM copy.
//
// Args:
//   g: display to draw on.
//   x: left pixel position.
//   y: baseline pixel position.
//   pstr: PSTR-wrapped, flash-resident string.
static void P(U8G2& g, int x, int y, const char* pstr) {
  char b[22];
  strncpy_P(b, pstr, sizeof(b));
  b[sizeof(b) - 1] = '\0';
  g.drawStr(x, y, b);
}

// Draws one value with edit emphasis: a flashing field while EDITING, a box
// while NAVIGATING, plain otherwise. Spec 4.5 requires the two states to
// look visibly different.
//
// Args:
//   g: display to draw on.
//   x: left pixel position.
//   y: baseline pixel position.
//   s: text to draw.
//   isCursor: whether this is the field under the cursor.
//   editing: whether the cursor field is in EDITING mode.
//   nowMs: current time, used to phase the blink.
static void drawField(U8G2& g, int x, int y, const char* s,
                      bool isCursor, bool editing, uint32_t nowMs) {
  bool show = true;
  if (isCursor && editing && !blinkOn(nowMs)) show = false;  // flash off-phase
  if (show) g.drawStr(x, y, s);
  if (isCursor && !editing) {
    int w = g.getStrWidth(s);
    g.drawFrame(x - 2, y - 11, w + 4, 14);
  }
}

// Draws an HH:MM AM/PM field triplet, occupying cursor positions 0 through 2
// of a row. Shared by drawSetAlarm and drawSetTime.
//
// Args:
//   g: display to draw on.
//   y: baseline pixel position for the row.
//   t: time to display.
//   cursor: index of the field under the cursor.
//   editing: whether the cursor field is in EDITING mode.
//   nowMs: current time, used to phase the blink.
static void drawClockFields(U8G2& g, int y, ClockHM t, uint8_t cursor,
                            bool editing, uint32_t nowMs) {
  char hh[6], mm[6];
  snprintf(hh, sizeof(hh), "%02d", t.hour12);
  snprintf(mm, sizeof(mm), "%02d", t.minute);
  drawField(g, 20, y, hh, cursor == 0, editing, nowMs);
  g.drawStr(40, y, ":");
  drawField(g, 50, y, mm, cursor == 1, editing, nowMs);
  drawField(g, 78, y, t.pm ? "PM" : "AM", cursor == 2, editing, nowMs);
}

void OledDriver::begin() {
  u8g2_.begin();
  u8g2_.setContrast(255);
  lastActivityMs_ = 0;
  dimmed_ = false;
}

bool OledDriver::updatePowerSave(bool activity, bool alarmActive,
                                 UIMenu::Screen screen, uint32_t nowMs) {
  if (activity) {
    lastActivityMs_ = nowMs;
    if (dimmed_) { u8g2_.setPowerSave(0); dimmed_ = false; }
  }
  bool homeIdle = (!alarmActive) && (screen == UIMenu::Screen::HOME) &&
                  ((nowMs - lastActivityMs_) > OLED_IDLE_MS);
  if (homeIdle && !dimmed_) { u8g2_.setPowerSave(1); dimmed_ = true; }
  if (dimmed_) {
    if (!homeIdle) { u8g2_.setPowerSave(0); dimmed_ = false; }  // safety re-wake
    else return true;                                           // panel off, skip draw
  }
  return false;
}

void OledDriver::render(const UIMenu& ui, bool lampOn, const char* lampName,
                        bool alarmActive, bool snoozing, uint32_t snoozeRemainingMs,
                        uint32_t nowMs, bool activity) {
  if (updatePowerSave(activity, alarmActive, ui.screen(), nowMs)) return;

  u8g2_.firstPage();
  do {
    if (alarmActive) {
      drawAlarm(snoozing, snoozeRemainingMs);
    } else {
      switch (ui.screen()) {
        case UIMenu::Screen::HOME:         drawHome(ui, lampOn, lampName); break;
        case UIMenu::Screen::MENU:         drawMenu(ui); break;
        case UIMenu::Screen::SET_ALARM:    drawSetAlarm(ui, nowMs); break;
        case UIMenu::Screen::SET_TIME:     drawSetTime(ui, nowMs); break;
        case UIMenu::Screen::COLOR_PRESET: drawColorPreset(ui); break;
      }
    }
  } while (u8g2_.nextPage());
}

// Draws the armed-alarm badge, top-right: "*6:30" when armed, nothing
// otherwise.
static void drawHomeAlarmBadge(U8G2& g, const UIMenu& ui) {
  if (!ui.armed()) return;
  char buf[24];
  ClockHM a = ui.committedAlarm();
  snprintf_P(buf, sizeof(buf), PSTR("*%d:%02d"), a.hour12, a.minute);
  int w = g.getStrWidth(buf);
  g.drawStr(128 - w, 10, buf);
}

// Draws the big centered time with a small AM/PM suffix. Leaves the font
// set to FONT_SMALL.
static void drawHomeClock(U8G2& g, ClockHM t) {
  char buf[24];
  snprintf_P(buf, sizeof(buf), PSTR("%d:%02d"), t.hour12, t.minute);
  g.setFont(FONT_BIG);
  int bw = g.getStrWidth(buf);
  int bx = (128 - bw) / 2 - 8;
  g.drawStr(bx, 40, buf);
  g.setFont(FONT_SMALL);
  g.drawStr(bx + bw + 2, 40, t.pm ? "PM" : "AM");
}

// Draws the date line, e.g. "Wed Jul 2 2026".
static void drawHomeDateLine(U8G2& g, CalDate d) {
  char buf[24], mon[4], day[4];
  name3(DOWS, dow(d.year, d.month, d.day), day);
  name3(MONS, d.month, mon);
  snprintf_P(buf, sizeof(buf), PSTR("%s %s %d %u"), day, mon, d.day, d.year);
  g.drawStr(0, 54, buf);
}

// Draws the lamp status line, e.g. "Lamp: Warm White" or "Lamp: off".
static void drawHomeLampLine(U8G2& g, bool lampOn, const char* lampName) {
  char buf[24];
  if (lampOn) snprintf_P(buf, sizeof(buf), PSTR("Lamp: %s"), lampName);
  else        strncpy_P(buf, PSTR("Lamp: off"), sizeof(buf));
  g.drawStr(0, 64, buf);
}

void OledDriver::drawHome(const UIMenu& ui, bool lampOn, const char* lampName) {
  u8g2_.setFont(FONT_SMALL);
  P(u8g2_, 0, 10, PSTR("HOME"));

  drawHomeAlarmBadge(u8g2_, ui);
  drawHomeClock(u8g2_, ui.liveClock());
  drawHomeDateLine(u8g2_, ui.liveDate());
  drawHomeLampLine(u8g2_, lampOn, lampName);
}

void OledDriver::drawMenu(const UIMenu& ui) {
  u8g2_.setFont(FONT_SMALL);
  P(u8g2_, 0, 10, PSTR("MENU"));
  for (uint8_t i = 0; i < UIMenu::MI_COUNT; ++i) {
    int y = 22 + i * 11;
    if (i == ui.menuIndex()) P(u8g2_, 0, y, PSTR(">"));
    switch (i) {
      case UIMenu::MI_SET_ALARM:    P(u8g2_, 12, y, PSTR("Set Alarm")); break;
      case UIMenu::MI_SET_TIME:     P(u8g2_, 12, y, PSTR("Set Time")); break;
      case UIMenu::MI_COLOR_PRESET: P(u8g2_, 12, y, PSTR("Color Preset")); break;
      case UIMenu::MI_ALARM_TOGGLE: P(u8g2_, 12, y, ui.armed() ? PSTR("Alarm: ON")
                                                               : PSTR("Alarm: OFF")); break;
      case UIMenu::MI_EXIT:         P(u8g2_, 12, y, PSTR("Exit")); break;
    }
  }
}

void OledDriver::drawSetAlarm(const UIMenu& ui, uint32_t nowMs) {
  ClockHM a = ui.alarmEdit();
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  U8G2& g = u8g2_;

  g.setFont(FONT_SMALL);
  P(g, 0, 10, PSTR("SET ALARM"));

  drawClockFields(g, 34, a, ui.cursor(), editing, nowMs);

  if (ui.onCancelField()) P(g, 0, 50, PSTR("> [Cancel]"));
  P(g, 0, 64, PSTR("press=next long=sv"));
}

void OledDriver::drawSetTime(const UIMenu& ui, uint32_t nowMs) {
  char dd[6], yy[8], mon[4];
  ClockHM t = ui.timeEdit();
  CalDate d = ui.dateEdit();
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  uint8_t c = ui.cursor();
  U8G2& g = u8g2_;

  g.setFont(FONT_SMALL);
  P(g, 0, 10, PSTR("SET TIME"));

  drawClockFields(g, 30, t, c, editing, nowMs);

  int y2 = 48;
  name3(MONS, d.month, mon);
  drawField(g, 8, y2, mon, c == 3, editing, nowMs);
  snprintf(dd, sizeof(dd), "%02d", d.day);
  drawField(g, 44, y2, dd, c == 4, editing, nowMs);
  snprintf(yy, sizeof(yy), "%u", d.year);
  drawField(g, 74, y2, yy, c == 5, editing, nowMs);

  if (ui.onCancelField()) P(g, 0, 64, PSTR("> [Cancel]"));
  else                    P(g, 0, 64, PSTR("press=next long=sv"));
}

void OledDriver::drawColorPreset(const UIMenu& ui) {
  char buf[24];
  u8g2_.setFont(FONT_SMALL);
  P(u8g2_, 0, 10, PSTR("COLOR PRESET"));
  snprintf_P(buf, sizeof(buf), PSTR("> %s"), COLOR_PRESETS[ui.presetCursor()].name);
  u8g2_.drawStr(4, 26, buf);
  P(u8g2_, 0, 44, PSTR("rotate = preview"));
  P(u8g2_, 0, 54, PSTR("long = save"));
  P(u8g2_, 0, 64, PSTR("short = cancel"));
}

void OledDriver::drawAlarm(bool snoozing, uint32_t snoozeRemainingMs) {
  u8g2_.setFont(FONT_SMALL);
  if (snoozing) {
    uint32_t s = (snoozeRemainingMs + 999) / 1000;  // round up to whole seconds
    char buf[16];
    snprintf_P(buf, sizeof(buf), PSTR("%lu:%02lu"),
               (unsigned long)(s / 60), (unsigned long)(s % 60));
    u8g2_.setFont(FONT_BIG);
    int w = u8g2_.getStrWidth(buf);
    u8g2_.drawStr((128 - w) / 2, 40, buf);
    u8g2_.setFont(FONT_SMALL);
    P(u8g2_, 0, 62, PSTR("SNOOZE"));
  } else {
    P(u8g2_, 30, 24, PSTR("* ALARM *"));
    P(u8g2_, 0, 44, PSTR("soft power = dismiss"));
    P(u8g2_, 0, 54, PSTR("hold = cancel"));
    P(u8g2_, 0, 64, PSTR("press = snooze 10m"));
  }
}
