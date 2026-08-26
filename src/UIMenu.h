#pragma once
#include <stdint.h>
#include "Config.h"

// The screen, menu, and field-edit state machine (spec 4.3-4.6).
//
// Pure logic: no hardware, no Arduino headers. It owns the UI state and the
// working copies being edited. It emits a Cmd for the main loop to apply
// (commit a time, toggle armed, preview or commit a preset, nudge
// brightness) and exposes a view-model (screen, cursor, editing flag, and
// values) that OledDriver renders. Testable natively in
// test/test_ui_menu.cpp.
//
// Editing grammar (spec 4.5): each field is NAVIGATING (steady) or EDITING
// (flashing). The first short press enters edit; each later short press
// confirms the field and advances, chaining straight into editing the next
// value field, and landing NAVIGATING on the [Cancel] pseudo-field after the
// last real one. A long press saves everything and exits; confirming
// [Cancel] discards.
//
// Color Preset uses the opposite grammar on purpose (spec 4.6): rotate
// previews live, long press saves, short press cancels or reverts.
//
// The alarm is authoritative: when it fires, the main loop calls
// abortToHome(), which drops any uncommitted edit (spec 4.5).
class UIMenu {
public:
  enum class Screen : uint8_t { HOME, MENU, SET_ALARM, SET_TIME, COLOR_PRESET };
  enum class Mode   : uint8_t { NAVIGATING, EDITING };

  // Menu rows (spec 4.4): Set Alarm / Set Time / Color Preset / Alarm On-Off / Exit
  enum MenuItem : uint8_t {
    MI_SET_ALARM, MI_SET_TIME, MI_COLOR_PRESET, MI_ALARM_TOGGLE, MI_EXIT, MI_COUNT
  };

  // Intents handed back to the main loop to apply against domain or hardware.
  enum class Cmd : uint8_t {
    NONE, ADJUST_BRIGHTNESS, COMMIT_ALARM, COMMIT_TIME, SET_ARMED,
    PREVIEW_PRESET, COMMIT_PRESET
  };
  struct Result {
    Cmd     cmd = Cmd::NONE;
    int16_t brightnessDelta = 0;   // ADJUST_BRIGHTNESS
    ClockHM alarm{0, 0, false};    // COMMIT_ALARM
    ClockHM tod{0, 0, false};      // COMMIT_TIME (time-of-day part)
    CalDate date{1, 1, YEAR_MIN};  // COMMIT_TIME (date part)
    bool    armed = false;         // SET_ARMED
    uint8_t preset = 0;            // PREVIEW_PRESET / COMMIT_PRESET
  };

  // Constructs a menu that boots on HOME with the default alarm and clock.
  UIMenu();

  // Seeds committed state from the RTC and EEPROM at boot.
  //
  // Args:
  //   alarm: committed alarm time.
  //   armed: whether the alarm is armed.
  //   preset: committed color preset index.
  //   clock: current time of day.
  //   date: current calendar date.
  void seed(ClockHM alarm, bool armed, uint8_t preset, ClockHM clock, CalDate date);

  // Refreshes the live clock. Called every frame; this is also the value
  // seeded into the editor when Set Time is entered.
  //
  // Args:
  //   clock: current time of day.
  //   date: current calendar date.
  void tick(ClockHM clock, CalDate date);

  // Handles an encoder rotation. Route this only when the alarm is not
  // active.
  //
  // Args:
  //   delta: signed step count from the encoder.
  //   lampOn: whether the manual lamp is currently on, which controls
  //       whether rotating on HOME adjusts brightness.
  // Returns:
  //   The command to apply, if this rotation produced one.
  Result onRotate(int16_t delta, bool lampOn);

  // Handles a short encoder press. Route this only when the alarm is not
  // active.
  //
  // Returns:
  //   The command to apply, if this press produced one.
  Result onShort();

  // Handles a long encoder press. Route this only when the alarm is not
  // active.
  //
  // Returns:
  //   The command to apply, if this press produced one.
  Result onLong();

  // Discards any uncommitted edit and returns to HOME, because the alarm
  // took over (spec 4.5).
  void abortToHome();

  Screen   screen() const       { return screen_; }
  Mode     mode() const         { return mode_; }
  uint8_t  cursor() const       { return cursor_; }        // field index in editors
  uint8_t  menuIndex() const    { return menuIndex_; }
  bool     armed() const        { return armed_; }
  uint8_t  presetCursor() const { return cursorPreset_; }   // Color Preset highlight
  ClockHM  liveClock() const    { return liveClock_; }
  CalDate  liveDate() const     { return liveDate_; }
  ClockHM  alarmEdit() const    { return alarmWC_; }
  ClockHM  timeEdit() const     { return todWC_; }
  CalDate  dateEdit() const     { return dateWC_; }
  ClockHM  committedAlarm() const { return committedAlarm_; }

  // Returns the index of the [Cancel] pseudo-field for the active editor
  // (one past the last real field).
  uint8_t  cancelIndex() const;
  bool     onCancelField() const { return cursor_ == cancelIndex(); }

private:
  // Returns the number of fields, including [Cancel], for the active editor.
  uint8_t fieldCount() const;

  // Opens Set Alarm, seeding the working copy from the committed alarm.
  void enterSetAlarm();

  // Opens Set Time, seeding the working copy from the live clock and date.
  void enterSetTime();

  // Handles a short press while on the MENU screen.
  //
  // Returns:
  //   The command to apply, if selecting this row produced one.
  Result handleMenuShort();

  // Handles a short press while in a value editor (Set Alarm or Set Time).
  //
  // Returns:
  //   Always the default result; editors only produce a command on commit.
  Result handleEditShort();

  // Mutates the working copy field under the cursor.
  //
  // Args:
  //   delta: signed step to apply to the field's value.
  void adjustValue(int16_t delta);

  // Moves the cursor between fields, wrapping at either end.
  //
  // Args:
  //   delta: signed step count from the encoder.
  void moveCursor(int16_t delta);

  Screen  screen_;
  Mode    mode_;
  uint8_t cursor_;
  uint8_t menuIndex_;

  // Committed (last-saved) domain state, mirrored here for display and for
  // seeding editors.
  ClockHM committedAlarm_;
  bool    armed_;
  uint8_t committedPreset_;
  ClockHM liveClock_;
  CalDate liveDate_;

  // Working copies while editing.
  ClockHM alarmWC_;
  ClockHM todWC_;
  CalDate dateWC_;
  uint8_t entryPreset_;   // preset when the picker opened, for revert
  uint8_t cursorPreset_;  // highlighted preset in the picker
};
