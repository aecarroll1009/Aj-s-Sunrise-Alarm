#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// UIMenu — screen/menu/field-edit state machine (spec SS 4.3-4.6).
//
// PURE LOGIC: no hardware, no Arduino headers. It owns the UI state and the
// working copies being edited; it emits a Cmd for the main loop to apply
// (commit a time, toggle armed, preview/commit a preset, nudge brightness) and
// exposes a view-model (screen + cursor + editing flag + values) that
// OledDriver renders. Native-testable (test/test_ui_menu.cpp).
//
// Editing grammar (spec SS 4.5): each field is NAVIGATING (steady) or EDITING
// (flashing). First short-press enters edit; each subsequent short-press
// confirms the field and advances -- chaining straight into editing the next
// value field, and landing NAVIGATING on the [Cancel] pseudo-field after the
// last real one. Long-press saves all and exits; confirming [Cancel] discards.
//
// Color Preset uses the DELIBERATELY INVERTED grammar (spec SS 4.6): rotate
// previews live, long-press saves, short-press cancels/reverts.
//
// The alarm is authoritative: when it fires the main loop calls abortToHome(),
// which drops any uncommitted edit (spec SS 4.5).
// ============================================================================

class UIMenu {
public:
  enum class Screen : uint8_t { HOME, MENU, SET_ALARM, SET_TIME, COLOR_PRESET };
  enum class Mode   : uint8_t { NAVIGATING, EDITING };

  // Menu rows (spec SS 4.4): Set Alarm / Set Time / Color Preset / Alarm On-Off / Exit
  enum MenuItem : uint8_t {
    MI_SET_ALARM, MI_SET_TIME, MI_COLOR_PRESET, MI_ALARM_TOGGLE, MI_EXIT, MI_COUNT
  };

  // Intents handed back to the main loop to apply against domain/hardware.
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

  UIMenu();

  // Seed committed state from RTC/EEPROM at boot.
  void seed(ClockHM alarm, bool armed, uint8_t preset, ClockHM clock, CalDate date);
  // Refresh the live clock each frame (also the seed used when entering Set Time).
  void tick(ClockHM clock, CalDate date);

  // Encoder events -- route here ONLY when the alarm is not active.
  Result onRotate(int16_t delta, bool lampOn);
  Result onShort();
  Result onLong();

  // Alarm took over: discard uncommitted edit, return to HOME (spec SS 4.5).
  void abortToHome();

  // --- view-model (for OledDriver) -----------------------------------------
  Screen   screen() const      { return screen_; }
  Mode     mode() const        { return mode_; }
  uint8_t  cursor() const       { return cursor_; }       // field index in editors
  uint8_t  menuIndex() const    { return menuIndex_; }
  bool     armed() const        { return armed_; }
  uint8_t  presetCursor() const { return cursorPreset_; }  // Color Preset highlight
  ClockHM  liveClock() const    { return liveClock_; }
  CalDate  liveDate() const     { return liveDate_; }
  ClockHM  alarmEdit() const    { return alarmWC_; }
  ClockHM  timeEdit() const     { return todWC_; }
  CalDate  dateEdit() const     { return dateWC_; }
  ClockHM  committedAlarm() const { return committedAlarm_; }
  // Index of the [Cancel] pseudo-field for the active editor (fields count - 1).
  uint8_t  cancelIndex() const;
  bool     onCancelField() const { return cursor_ == cancelIndex(); }

private:
  uint8_t fieldCount() const;             // per active editor screen
  void    enterSetAlarm();
  void    enterSetTime();
  Result  handleMenuShort();
  Result  handleEditShort();
  void    adjustValue(int16_t delta);     // mutate the working copy field under cursor
  void    moveCursor(int16_t delta);

  Screen  screen_;
  Mode    mode_;
  uint8_t cursor_;
  uint8_t menuIndex_;

  // committed (last-saved) domain state mirrored here for display + editor seeding
  ClockHM committedAlarm_;
  bool    armed_;
  uint8_t committedPreset_;
  ClockHM liveClock_;
  CalDate liveDate_;

  // working copies while editing
  ClockHM alarmWC_;
  ClockHM todWC_;
  CalDate dateWC_;
  uint8_t entryPreset_;   // preset when the picker opened (for revert)
  uint8_t cursorPreset_;  // highlighted preset in the picker
};
