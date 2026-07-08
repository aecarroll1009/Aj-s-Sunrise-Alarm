#include "OledDriver.h"
#include <Arduino.h>
#include <stdio.h>

// Fonts kept in one place so bring-up tuning is a one-line change.
#define FONT_SMALL u8g2_font_6x12_tf       // menus, labels, hints
#define FONT_BIG   u8g2_font_logisoso24_tn // big HOME clock (digits + ':')

namespace {

// 3-char names packed in flash; index = value*3. Saves RAM vs const char*[].
const char MONS[] PROGMEM = "---JanFebMarAprMayJunJulAugSepOctNovDec";
const char DOWS[] PROGMEM = "SunMonTueWedThuFriSat";

void name3(const char* table, uint8_t idx, char out[4]) {
  memcpy_P(out, table + (uint16_t)idx * 3, 3);
  out[3] = '\0';
}

// Day-of-week (0=Sun) via Sakamoto's algorithm; RTC only gives Y/M/D.
uint8_t dow(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}

bool blinkOn(uint32_t nowMs) { return ((nowMs / EDIT_FLASH_MS) & 1) == 0; }

} // namespace

// Draw a flash-resident (PSTR) label without a permanent RAM copy.
static void P(U8G2& g, int x, int y, const char* pstr) {
  char b[22];
  strncpy_P(b, pstr, sizeof(b));
  b[sizeof(b) - 1] = '\0';
  g.drawStr(x, y, b);
}

void OledDriver::begin() {
  u8g2_.begin();
  u8g2_.setContrast(255);
  lastActivityMs_ = 0;
  dimmed_ = false;
}

void OledDriver::render(const UIMenu& ui, bool lampOn, const char* lampName,
                        bool alarmActive, bool snoozing, uint32_t snoozeRemainingMs,
                        uint32_t nowMs, bool activity) {
  // Night dimming applies only on the HOME screen (spec SS 4.10); anything else
  // (a menu, an active alarm) stays awake.
  if (activity) {
    lastActivityMs_ = nowMs;
    if (dimmed_) { u8g2_.setPowerSave(0); dimmed_ = false; }
  }
  bool homeIdle = (!alarmActive) && (ui.screen() == UIMenu::Screen::HOME) &&
                  ((nowMs - lastActivityMs_) > OLED_IDLE_MS);
  if (homeIdle && !dimmed_) { u8g2_.setPowerSave(1); dimmed_ = true; }
  if (dimmed_) {
    if (!homeIdle) { u8g2_.setPowerSave(0); dimmed_ = false; }  // safety re-wake
    else return;                                                // panel off, skip draw
  }

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

void OledDriver::drawHome(const UIMenu& ui, bool lampOn, const char* lampName) {
  char buf[24], mon[4], day[4];
  ClockHM t = ui.liveClock();
  CalDate d = ui.liveDate();

  u8g2_.setFont(FONT_SMALL);
  P(u8g2_, 0, 10, PSTR("HOME"));

  // alarm status, top-right: "*6:30" when armed
  if (ui.armed()) {
    ClockHM a = ui.committedAlarm();
    snprintf_P(buf, sizeof(buf), PSTR("*%d:%02d"), a.hour12, a.minute);
    int w = u8g2_.getStrWidth(buf);
    u8g2_.drawStr(128 - w, 10, buf);
  }

  // big time, centered
  snprintf_P(buf, sizeof(buf), PSTR("%d:%02d"), t.hour12, t.minute);
  u8g2_.setFont(FONT_BIG);
  int bw = u8g2_.getStrWidth(buf);
  int bx = (128 - bw) / 2 - 8;
  u8g2_.drawStr(bx, 40, buf);
  u8g2_.setFont(FONT_SMALL);
  u8g2_.drawStr(bx + bw + 2, 40, t.pm ? "PM" : "AM");

  // date line: "Wed Jul 2 2026"
  name3(DOWS, dow(d.year, d.month, d.day), day);   // reuse `day` for the DOW name
  name3(MONS, d.month, mon);
  snprintf_P(buf, sizeof(buf), PSTR("%s %s %d %u"), day, mon, d.day, d.year);
  u8g2_.drawStr(0, 54, buf);

  if (lampOn) snprintf_P(buf, sizeof(buf), PSTR("Lamp: %s"), lampName);
  else        strncpy_P(buf, PSTR("Lamp: off"), sizeof(buf));
  u8g2_.drawStr(0, 64, buf);
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

// Draw a value with edit emphasis: EDITING flashes, NAVIGATING gets a box,
// otherwise plain (spec SS 4.5: the two states must look different).
static void drawField(U8G2& g, int x, int y, const char* s,
                      bool isCursor, bool editing, uint32_t nowMs) {
  bool show = true;
  if (isCursor && editing && !blinkOn(nowMs)) show = false;  // flash off-phase
  if (show) g.drawStr(x, y, s);
  if (isCursor && !editing) {                                // navigating: box it
    int w = g.getStrWidth(s);
    g.drawFrame(x - 2, y - 11, w + 4, 14);
  }
}

void OledDriver::drawSetAlarm(const UIMenu& ui, uint32_t nowMs) {
  char hh[6], mm[6];
  ClockHM a = ui.alarmEdit();
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  uint8_t c = ui.cursor();
  U8G2& g = u8g2_;

  g.setFont(FONT_SMALL);
  P(g, 0, 10, PSTR("SET ALARM"));

  snprintf(hh, sizeof(hh), "%02d", a.hour12);
  snprintf(mm, sizeof(mm), "%02d", a.minute);
  int y = 34;
  drawField(g, 20, y, hh, c == 0, editing, nowMs);
  g.drawStr(40, y, ":");
  drawField(g, 50, y, mm, c == 1, editing, nowMs);
  drawField(g, 78, y, a.pm ? "PM" : "AM", c == 2, editing, nowMs);

  if (ui.onCancelField()) P(g, 0, 50, PSTR("> [Cancel]"));
  P(g, 0, 64, PSTR("press=next long=sv"));
}

void OledDriver::drawSetTime(const UIMenu& ui, uint32_t nowMs) {
  char hh[6], mm[6], dd[6], yy[8], mon[4];
  ClockHM t = ui.timeEdit();
  CalDate d = ui.dateEdit();
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  uint8_t c = ui.cursor();
  U8G2& g = u8g2_;

  g.setFont(FONT_SMALL);
  P(g, 0, 10, PSTR("SET TIME"));

  snprintf(hh, sizeof(hh), "%02d", t.hour12);
  snprintf(mm, sizeof(mm), "%02d", t.minute);
  int y1 = 30;
  drawField(g, 20, y1, hh, c == 0, editing, nowMs);
  g.drawStr(40, y1, ":");
  drawField(g, 50, y1, mm, c == 1, editing, nowMs);
  drawField(g, 78, y1, t.pm ? "PM" : "AM", c == 2, editing, nowMs);

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
    uint32_t s = (snoozeRemainingMs + 999) / 1000;   // round up to whole seconds
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
