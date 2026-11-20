#pragma once
#include <stdint.h>
#include "Tuning.h"
#include "Types.h"

constexpr uint8_t  UI_BRIGHTNESS_STEP = 12;  // per encoder detent on HOME
constexpr uint16_t YEAR_MIN = 2024;
constexpr uint16_t YEAR_MAX = 2099;

// Screens, menu and value editors (spec 4.3-4.6). Pure logic.
//
// Handlers return a Result naming what to save; the sketch applies it.
// Editors: short press edits a field, then confirms and moves to the next;
// long press saves; confirming [Cancel] discards. Color Preset: turning
// previews, long press saves, short press discards.
class UIMenu {
 public:
  enum class Screen : uint8_t { HOME, MENU, SET_ALARM, SET_TIME, COLOR_PRESET };
  enum class Mode   : uint8_t { NAVIGATING, EDITING };

  // MENU rows, top to bottom.
  enum MenuItem : uint8_t {
    MI_SET_ALARM, MI_SET_TIME, MI_COLOR_PRESET, MI_ALARM_TOGGLE, MI_EXIT, MI_COUNT
  };

  enum class Cmd : uint8_t {
    NONE, ADJUST_BRIGHTNESS, SAVE_ALARM, SAVE_TIME, SET_ARMED, SAVE_PRESET
  };

  // What to apply after an input. Only the fields for cmd are set.
  struct Result {
    Cmd     cmd = Cmd::NONE;
    int16_t brightnessDelta = 0;
    ClockHM alarm{0, 0, false};
    ClockHM time{0, 0, false};
    CalDate date{1, 1, YEAR_MIN};
    bool    armed = false;
    uint8_t preset = 0;
  };

  // Loads the saved settings at boot.
  void load(ClockHM alarm, bool armed, uint8_t preset);

  // Updates the current time; call every frame.
  void setNow(ClockHM clock, CalDate date);

  // Handles an encoder turn.
  //
  // Args:
  //   delta: signed detents.
  //   lampOn: whether turning on HOME should adjust brightness.
  Result onRotate(int16_t delta, bool lampOn);

  Result onShort();
  Result onLong();

  // Drops any unsaved edit and returns to HOME.
  void abortToHome();

  Screen   screen() const       { return screen_; }
  Mode     mode() const         { return mode_; }
  uint8_t  cursor() const       { return cursor_; }
  uint8_t  menuIndex() const    { return menuIndex_; }
  bool     armed() const        { return armed_; }
  ClockHM  alarm() const        { return alarm_; }
  uint8_t  presetCursor() const { return presetCursor_; }
  ClockHM  clock() const        { return clock_; }
  CalDate  date() const         { return date_; }
  ClockHM  alarmEdit() const    { return alarmEdit_; }
  ClockHM  timeEdit() const     { return timeEdit_; }
  CalDate  dateEdit() const     { return dateEdit_; }

  bool onCancelField() const { return cursor_ == fieldCount() - 1; }

 private:
  // Returns the open editor's field count, including [Cancel].
  uint8_t fieldCount() const;

  // Opens a screen with the cursor on its first field.
  void open(Screen s);

  Result selectMenuItem();
  void   advanceField();
  void   adjustField(int16_t delta);

  Screen  screen_ = Screen::HOME;
  Mode    mode_ = Mode::NAVIGATING;
  uint8_t cursor_ = 0;
  uint8_t menuIndex_ = 0;

  // Saved settings.
  ClockHM alarm_{6, 30, false};
  bool    armed_ = true;
  uint8_t preset_ = 0;

  ClockHM clock_{12, 0, false};
  CalDate date_{1, 1, YEAR_MIN};

  // Values being edited.
  ClockHM alarmEdit_{6, 30, false};
  ClockHM timeEdit_{12, 0, false};
  CalDate dateEdit_{1, 1, YEAR_MIN};
  uint8_t presetCursor_ = 0;
};
