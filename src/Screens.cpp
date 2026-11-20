#include "Screens.h"
#include <Arduino.h>
#include <stdio.h>
#include "Tuning.h"

#define FONT_SMALL u8g2_font_6x12_tf
#define FONT_BIG   u8g2_font_logisoso24_tn  // clock digits

namespace {

constexpr uint16_t BLINK_MS = 500;  // edited-field blink period

// Three-letter names in flash.
const char MONTHS[] PROGMEM   = "---JanFebMarAprMayJunJulAugSepOctNovDec";
const char WEEKDAYS[] PROGMEM = "SunMonTueWedThuFriSat";

// Copies name idx from a flash table into out.
void name3(const char* table, uint8_t idx, char out[4]) {
  memcpy_P(out, table + idx * 3, 3);
  out[3] = '\0';
}

// Returns the weekday, 0 for Sunday (Sakamoto's method).
uint8_t weekday(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}

// Draws a PSTR flash string.
void drawP(U8G2& g, int x, int y, const char* pstr) {
  char buf[22];
  strncpy_P(buf, pstr, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  g.drawStr(x, y, buf);
}

// Draws an editor field: boxed when selected, blinking while edited
// (spec 4.5).
void drawField(U8G2& g, int x, int y, const char* text, bool isCursor,
               bool editing, uint32_t nowMs) {
  bool blinkOff = isCursor && editing && (nowMs / BLINK_MS) % 2 == 1;
  if (!blinkOff) g.drawStr(x, y, text);
  if (isCursor && !editing) g.drawFrame(x - 2, y - 11, g.getStrWidth(text) + 4, 14);
}

// Draws HH:MM AM/PM as cursor fields 0-2.
void drawClockFields(U8G2& g, int y, ClockHM t, uint8_t cursor, bool editing,
                     uint32_t nowMs) {
  char hh[3], mm[3];
  snprintf(hh, sizeof(hh), "%02d", t.hour12);
  snprintf(mm, sizeof(mm), "%02d", t.minute);
  drawField(g, 20, y, hh, cursor == 0, editing, nowMs);
  g.drawStr(40, y, ":");
  drawField(g, 50, y, mm, cursor == 1, editing, nowMs);
  drawField(g, 78, y, t.pm ? "PM" : "AM", cursor == 2, editing, nowMs);
}

void drawHome(U8G2& g, const UIMenu& ui, const DisplayStatus& s) {
  char buf[24];
  g.setFont(FONT_SMALL);
  drawP(g, 0, 10, PSTR("HOME"));

  if (ui.armed()) {  // e.g. "*6:30", top right
    snprintf_P(buf, sizeof(buf), PSTR("*%d:%02d"), ui.alarm().hour12, ui.alarm().minute);
    g.drawStr(128 - g.getStrWidth(buf), 10, buf);
  }

  ClockHM t = ui.clock();
  snprintf_P(buf, sizeof(buf), PSTR("%d:%02d"), t.hour12, t.minute);
  g.setFont(FONT_BIG);
  int w = g.getStrWidth(buf);
  int x = (128 - w) / 2 - 8;
  g.drawStr(x, 40, buf);
  g.setFont(FONT_SMALL);
  g.drawStr(x + w + 2, 40, t.pm ? "PM" : "AM");

  CalDate d = ui.date();  // e.g. "Wed Jul 2 2026"
  char day[4], mon[4];
  name3(WEEKDAYS, weekday(d.year, d.month, d.day), day);
  name3(MONTHS, d.month, mon);
  snprintf_P(buf, sizeof(buf), PSTR("%s %s %d %u"), day, mon, d.day, d.year);
  g.drawStr(0, 54, buf);

  if (s.lampOn) snprintf_P(buf, sizeof(buf), PSTR("Lamp: %s"), COLOR_PRESETS[s.lampPreset].name);
  else          strncpy_P(buf, PSTR("Lamp: off"), sizeof(buf));
  g.drawStr(0, 64, buf);
}

void drawMenu(U8G2& g, const UIMenu& ui) {
  g.setFont(FONT_SMALL);
  drawP(g, 0, 10, PSTR("MENU"));
  for (uint8_t i = 0; i < UIMenu::MI_COUNT; ++i) {
    int y = 22 + i * 11;
    if (i == ui.menuIndex()) drawP(g, 0, y, PSTR(">"));
    const char* label = PSTR("Exit");
    if (i == UIMenu::MI_SET_ALARM)    label = PSTR("Set Alarm");
    if (i == UIMenu::MI_SET_TIME)     label = PSTR("Set Time");
    if (i == UIMenu::MI_COLOR_PRESET) label = PSTR("Color Preset");
    if (i == UIMenu::MI_ALARM_TOGGLE) label = ui.armed() ? PSTR("Alarm: ON") : PSTR("Alarm: OFF");
    drawP(g, 12, y, label);
  }
}

void drawSetAlarm(U8G2& g, const UIMenu& ui, uint32_t nowMs) {
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  g.setFont(FONT_SMALL);
  drawP(g, 0, 10, PSTR("SET ALARM"));
  drawClockFields(g, 34, ui.alarmEdit(), ui.cursor(), editing, nowMs);
  if (ui.onCancelField()) drawP(g, 0, 50, PSTR("> [Cancel]"));
  drawP(g, 0, 64, PSTR("press=next long=sv"));
}

void drawSetTime(U8G2& g, const UIMenu& ui, uint32_t nowMs) {
  bool editing = (ui.mode() == UIMenu::Mode::EDITING);
  uint8_t c = ui.cursor();
  CalDate d = ui.dateEdit();
  char mon[4], dd[3], yyyy[5];
  name3(MONTHS, d.month, mon);
  snprintf(dd, sizeof(dd), "%02d", d.day);
  snprintf(yyyy, sizeof(yyyy), "%u", d.year);

  g.setFont(FONT_SMALL);
  drawP(g, 0, 10, PSTR("SET TIME"));
  drawClockFields(g, 30, ui.timeEdit(), c, editing, nowMs);
  drawField(g, 8, 48, mon, c == 3, editing, nowMs);
  drawField(g, 44, 48, dd, c == 4, editing, nowMs);
  drawField(g, 74, 48, yyyy, c == 5, editing, nowMs);
  drawP(g, 0, 64, ui.onCancelField() ? PSTR("> [Cancel]") : PSTR("press=next long=sv"));
}

void drawColorPreset(U8G2& g, const UIMenu& ui) {
  char buf[24];
  g.setFont(FONT_SMALL);
  drawP(g, 0, 10, PSTR("COLOR PRESET"));
  snprintf_P(buf, sizeof(buf), PSTR("> %s"), COLOR_PRESETS[ui.presetCursor()].name);
  g.drawStr(4, 26, buf);
  drawP(g, 0, 44, PSTR("rotate = preview"));
  drawP(g, 0, 54, PSTR("long = save"));
  drawP(g, 0, 64, PSTR("short = cancel"));
}

void drawAlarm(U8G2& g, const DisplayStatus& s) {
  g.setFont(FONT_SMALL);
  if (!s.snoozing) {
    drawP(g, 30, 24, PSTR("* ALARM *"));
    drawP(g, 0, 44, PSTR("soft power = dismiss"));
    drawP(g, 0, 54, PSTR("hold = cancel"));
    drawP(g, 0, 64, PSTR("press = snooze 10m"));
    return;
  }
  uint32_t secs = (s.snoozeRemainingMs + 999) / 1000;  // round up
  char buf[12];
  snprintf_P(buf, sizeof(buf), PSTR("%lu:%02lu"),
             (unsigned long)(secs / 60), (unsigned long)(secs % 60));
  g.setFont(FONT_BIG);
  g.drawStr((128 - g.getStrWidth(buf)) / 2, 40, buf);
  g.setFont(FONT_SMALL);
  drawP(g, 0, 62, PSTR("SNOOZE"));
}

}  // namespace

namespace Screens {

void draw(U8G2& g, const UIMenu& ui, const DisplayStatus& s, uint32_t nowMs) {
  if (s.alarmActive) {
    drawAlarm(g, s);
    return;
  }
  switch (ui.screen()) {
    case UIMenu::Screen::HOME:         drawHome(g, ui, s); break;
    case UIMenu::Screen::MENU:         drawMenu(g, ui); break;
    case UIMenu::Screen::SET_ALARM:    drawSetAlarm(g, ui, nowMs); break;
    case UIMenu::Screen::SET_TIME:     drawSetTime(g, ui, nowMs); break;
    case UIMenu::Screen::COLOR_PRESET: drawColorPreset(g, ui); break;
  }
}

}  // namespace Screens
