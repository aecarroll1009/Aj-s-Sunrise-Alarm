#include "UIMenu.h"

namespace {

// Editor fields in cursor order. Set Alarm stops after AMPM; [Cancel]
// follows the last field.
enum Field : uint8_t { HOUR, MINUTE, AMPM, MONTH, DAY, YEAR };

// Wraps v into lo..hi inclusive.
int16_t wrap(int16_t v, int16_t lo, int16_t hi) {
  int16_t span = hi - lo + 1;
  int16_t m = (v - lo) % span;
  if (m < 0) m += span;
  return lo + m;
}

// Steps one time-of-day field.
void stepClock(ClockHM& t, uint8_t field, int16_t delta) {
  if (field == HOUR)   t.hour12 = (uint8_t)wrap(t.hour12 + delta, 1, 12);
  if (field == MINUTE) t.minute = (uint8_t)wrap(t.minute + delta, 0, 59);
  if (field == AMPM && delta != 0) t.pm = !t.pm;
}

// Steps one date field.
void stepDate(CalDate& d, uint8_t field, int16_t delta) {
  if (field == MONTH) d.month = (uint8_t)wrap(d.month + delta, 1, 12);
  if (field == DAY)   d.day   = (uint8_t)wrap(d.day + delta, 1, 31);
  if (field == YEAR)  d.year  = (uint16_t)wrap(d.year + delta, YEAR_MIN, YEAR_MAX);
}

}  // namespace

void UIMenu::load(ClockHM alarm, bool armed, uint8_t preset) {
  alarm_  = alarm;
  armed_  = armed;
  preset_ = preset;
}

void UIMenu::setNow(ClockHM clock, CalDate date) {
  clock_ = clock;
  date_  = date;
}

uint8_t UIMenu::fieldCount() const {
  return (screen_ == Screen::SET_TIME) ? YEAR + 2 : AMPM + 2;
}

void UIMenu::open(Screen s) {
  screen_ = s;
  mode_   = Mode::NAVIGATING;
  cursor_ = 0;
}

void UIMenu::abortToHome() { open(Screen::HOME); }

void UIMenu::adjustField(int16_t delta) {
  if (screen_ == Screen::SET_ALARM) {
    stepClock(alarmEdit_, cursor_, delta);
  } else {
    stepClock(timeEdit_, cursor_, delta);
    stepDate(dateEdit_, cursor_, delta);
  }
}

UIMenu::Result UIMenu::onRotate(int16_t delta, bool lampOn) {
  Result r;
  switch (screen_) {
    case Screen::HOME:
      if (lampOn && delta != 0) {
        r.cmd = Cmd::ADJUST_BRIGHTNESS;
        r.brightnessDelta = delta * (int16_t)UI_BRIGHTNESS_STEP;
      }
      break;
    case Screen::MENU:
      menuIndex_ = (uint8_t)wrap(menuIndex_ + delta, 0, MI_COUNT - 1);
      break;
    case Screen::SET_ALARM:
    case Screen::SET_TIME:
      if (mode_ == Mode::EDITING) adjustField(delta);
      else cursor_ = (uint8_t)wrap(cursor_ + delta, 0, fieldCount() - 1);
      break;
    case Screen::COLOR_PRESET:
      presetCursor_ = (uint8_t)wrap(presetCursor_ + delta, 0, COLOR_PRESET_COUNT - 1);
      break;
  }
  return r;
}

UIMenu::Result UIMenu::selectMenuItem() {
  Result r;
  switch (menuIndex_) {
    case MI_SET_ALARM:
      alarmEdit_ = alarm_;
      open(Screen::SET_ALARM);
      break;
    case MI_SET_TIME:
      timeEdit_ = clock_;
      dateEdit_ = date_;
      open(Screen::SET_TIME);
      break;
    case MI_COLOR_PRESET:
      presetCursor_ = preset_;
      open(Screen::COLOR_PRESET);
      break;
    case MI_ALARM_TOGGLE:
      armed_  = !armed_;
      r.cmd   = Cmd::SET_ARMED;
      r.armed = armed_;
      break;
    case MI_EXIT:
      open(Screen::HOME);
      break;
  }
  return r;
}

void UIMenu::advanceField() {
  if (mode_ == Mode::NAVIGATING) {
    if (onCancelField()) open(Screen::MENU);
    else mode_ = Mode::EDITING;
    return;
  }
  ++cursor_;
  if (onCancelField()) mode_ = Mode::NAVIGATING;
}

UIMenu::Result UIMenu::onShort() {
  Result r;
  switch (screen_) {
    case Screen::HOME:
      menuIndex_ = 0;
      open(Screen::MENU);
      break;
    case Screen::MENU:
      r = selectMenuItem();
      break;
    case Screen::SET_ALARM:
    case Screen::SET_TIME:
      advanceField();
      break;
    case Screen::COLOR_PRESET:
      open(Screen::MENU);
      break;
  }
  return r;
}

UIMenu::Result UIMenu::onLong() {
  Result r;
  switch (screen_) {
    case Screen::SET_ALARM:
      alarm_  = alarmEdit_;
      r.cmd   = Cmd::SAVE_ALARM;
      r.alarm = alarm_;
      break;
    case Screen::SET_TIME:
      r.cmd  = Cmd::SAVE_TIME;
      r.time = timeEdit_;
      r.date = dateEdit_;
      break;
    case Screen::COLOR_PRESET:
      preset_  = presetCursor_;
      r.cmd    = Cmd::SAVE_PRESET;
      r.preset = preset_;
      break;
    case Screen::MENU:
      open(Screen::HOME);
      return r;
    case Screen::HOME:
      return r;
  }
  open(Screen::MENU);
  return r;
}
