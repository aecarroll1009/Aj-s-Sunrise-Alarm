#include "UIMenu.h"

namespace {

// Field layout per editor (index order = navigation/advance order).
// Set Alarm:  HH MM AMPM [Cancel]
// Set Time:   HH MM AMPM MONTH DAY YEAR [Cancel]
enum { F_HH = 0, F_MM = 1, F_AMPM = 2 };
enum { T_MONTH = 3, T_DAY = 4, T_YEAR = 5 };
constexpr uint8_t ALARM_FIELDS = 4;   // 3 real + Cancel
constexpr uint8_t TIME_FIELDS  = 7;   // 6 real + Cancel

uint8_t wrapIdx(int16_t v, uint8_t count) {
  if (count == 0) return 0;
  int16_t m = v % (int16_t)count;
  if (m < 0) m += count;
  return (uint8_t)m;
}
uint8_t wrap1_12(int16_t v) { return (uint8_t)(wrapIdx(v - 1, 12) + 1); }
uint8_t wrap0_59(int16_t v) { return (uint8_t)wrapIdx(v, 60); }
uint8_t wrap1_31(int16_t v) { return (uint8_t)(wrapIdx(v - 1, 31) + 1); }

uint16_t wrapYear(int16_t v) {
  const int16_t span = (int16_t)(YEAR_MAX - YEAR_MIN + 1);
  return (uint16_t)(YEAR_MIN + wrapIdx(v - (int16_t)YEAR_MIN, (uint8_t)span));
}

} // namespace

UIMenu::UIMenu()
    : screen_(Screen::HOME), mode_(Mode::NAVIGATING), cursor_(0), menuIndex_(0),
      committedAlarm_{6, 30, false}, armed_(true), committedPreset_(0),
      liveClock_{12, 0, false}, liveDate_{1, 1, YEAR_MIN},
      alarmWC_{6, 30, false}, todWC_{12, 0, false}, dateWC_{1, 1, YEAR_MIN},
      entryPreset_(0), cursorPreset_(0) {}

void UIMenu::seed(ClockHM alarm, bool armed, uint8_t preset, ClockHM clock, CalDate date) {
  committedAlarm_  = alarm;
  armed_           = armed;
  committedPreset_ = preset;
  liveClock_       = clock;
  liveDate_        = date;
}

void UIMenu::tick(ClockHM clock, CalDate date) {
  liveClock_ = clock;
  liveDate_  = date;
}

uint8_t UIMenu::fieldCount() const {
  return (screen_ == Screen::SET_TIME) ? TIME_FIELDS : ALARM_FIELDS;
}
uint8_t UIMenu::cancelIndex() const { return fieldCount() - 1; }

void UIMenu::enterSetAlarm() {
  alarmWC_ = committedAlarm_;
  screen_  = Screen::SET_ALARM;
  cursor_  = 0;
  mode_    = Mode::NAVIGATING;
}

void UIMenu::enterSetTime() {
  todWC_  = liveClock_;   // start from the live clock
  dateWC_ = liveDate_;
  screen_ = Screen::SET_TIME;
  cursor_ = 0;
  mode_   = Mode::NAVIGATING;
}

void UIMenu::abortToHome() {
  screen_ = Screen::HOME;   // working copies simply not committed
  mode_   = Mode::NAVIGATING;
  cursor_ = 0;
}

void UIMenu::moveCursor(int16_t delta) {
  cursor_ = wrapIdx((int16_t)cursor_ + delta, fieldCount());
}

void UIMenu::adjustValue(int16_t delta) {
  if (screen_ == Screen::SET_ALARM) {
    switch (cursor_) {
      case F_HH:   alarmWC_.hour12 = wrap1_12((int16_t)alarmWC_.hour12 + delta); break;
      case F_MM:   alarmWC_.minute = wrap0_59((int16_t)alarmWC_.minute + delta); break;
      case F_AMPM: if (delta) alarmWC_.pm = !alarmWC_.pm; break;
      default: break;
    }
  } else if (screen_ == Screen::SET_TIME) {
    switch (cursor_) {
      case F_HH:    todWC_.hour12 = wrap1_12((int16_t)todWC_.hour12 + delta); break;
      case F_MM:    todWC_.minute = wrap0_59((int16_t)todWC_.minute + delta); break;
      case F_AMPM:  if (delta) todWC_.pm = !todWC_.pm; break;
      case T_MONTH: dateWC_.month = wrap1_12((int16_t)dateWC_.month + delta); break;
      case T_DAY:   dateWC_.day   = wrap1_31((int16_t)dateWC_.day + delta); break;
      case T_YEAR:  dateWC_.year  = wrapYear((int16_t)dateWC_.year + delta); break;
      default: break;
    }
  }
}

UIMenu::Result UIMenu::onRotate(int16_t delta, bool lampOn) {
  Result r;
  switch (screen_) {
    case Screen::HOME:
      if (lampOn && delta != 0) {           // rotate = brightness when lamp on (spec SS 4.2/4.3)
        r.cmd = Cmd::ADJUST_BRIGHTNESS;
        r.brightnessDelta = delta * (int16_t)UI_BRIGHTNESS_STEP;
      }
      break;
    case Screen::MENU:
      menuIndex_ = wrapIdx((int16_t)menuIndex_ + delta, MI_COUNT);
      break;
    case Screen::SET_ALARM:
    case Screen::SET_TIME:
      if (mode_ == Mode::EDITING) adjustValue(delta);   // flashing field: change value
      else                        moveCursor(delta);    // navigating: move between fields
      break;
    case Screen::COLOR_PRESET:
      cursorPreset_ = wrapIdx((int16_t)cursorPreset_ + delta, COLOR_PRESET_COUNT);
      r.cmd = Cmd::PREVIEW_PRESET;                        // live preview on the strip
      r.preset = cursorPreset_;
      break;
  }
  return r;
}

UIMenu::Result UIMenu::handleMenuShort() {
  Result r;
  switch (menuIndex_) {
    case MI_SET_ALARM:    enterSetAlarm(); break;
    case MI_SET_TIME:     enterSetTime(); break;
    case MI_COLOR_PRESET:
      entryPreset_  = committedPreset_;
      cursorPreset_ = committedPreset_;
      screen_       = Screen::COLOR_PRESET;
      break;
    case MI_ALARM_TOGGLE:
      armed_   = !armed_;
      r.cmd    = Cmd::SET_ARMED;
      r.armed  = armed_;
      break;                                 // stay in the menu
    case MI_EXIT:
      screen_ = Screen::HOME;
      break;
  }
  return r;
}

UIMenu::Result UIMenu::handleEditShort() {
  Result r;
  if (mode_ == Mode::NAVIGATING) {
    if (cursor_ == cancelIndex()) {
      screen_ = Screen::MENU;                // confirm [Cancel] -> discard + exit
    } else {
      mode_ = Mode::EDITING;                 // first short-press enters edit
    }
  } else {                                   // EDITING: confirm current, advance
    ++cursor_;
    mode_ = (cursor_ == cancelIndex()) ? Mode::NAVIGATING : Mode::EDITING;
  }
  return r;
}

UIMenu::Result UIMenu::onShort() {
  Result r;
  switch (screen_) {
    case Screen::HOME:
      screen_ = Screen::MENU;
      menuIndex_ = 0;
      break;
    case Screen::MENU:
      r = handleMenuShort();
      break;
    case Screen::SET_ALARM:
    case Screen::SET_TIME:
      r = handleEditShort();
      break;
    case Screen::COLOR_PRESET:               // short = cancel/revert (spec SS 4.6)
      r.cmd = Cmd::PREVIEW_PRESET;
      r.preset = entryPreset_;               // revert the strip to the entry color
      screen_ = Screen::MENU;
      break;
  }
  return r;
}

UIMenu::Result UIMenu::onLong() {
  Result r;
  switch (screen_) {
    case Screen::SET_ALARM:
      committedAlarm_ = alarmWC_;            // save all
      r.cmd = Cmd::COMMIT_ALARM;
      r.alarm = alarmWC_;
      screen_ = Screen::MENU;
      break;
    case Screen::SET_TIME:
      r.cmd = Cmd::COMMIT_TIME;
      r.tod = todWC_;
      r.date = dateWC_;
      screen_ = Screen::MENU;
      break;
    case Screen::COLOR_PRESET:
      committedPreset_ = cursorPreset_;      // save selected color
      r.cmd = Cmd::COMMIT_PRESET;
      r.preset = cursorPreset_;
      screen_ = Screen::MENU;
      break;
    case Screen::MENU:
      screen_ = Screen::HOME;
      break;
    case Screen::HOME:
      break;
  }
  return r;
}
