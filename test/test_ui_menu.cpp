// Native (host g++) unit test for UIMenu — runs without any hardware.
//
//   g++ -std=c++11 -I. -Wall -Wextra UIMenu.cpp test/test_ui_menu.cpp -o test/run_ui && test/run_ui

#include <cstdio>
#include "../UIMenu.h"

static int failures = 0;
#define CHECK(cond, msg) do {                                   \
    if (!(cond)) { printf("  FAIL: %s\n", msg); ++failures; }   \
    else         { printf("  ok:   %s\n", msg); }               \
  } while (0)

using S = UIMenu::Screen;
using M = UIMenu::Mode;
using C = UIMenu::Cmd;

static UIMenu freshAtMenu(uint8_t item) {
  UIMenu ui;
  ui.seed(ClockHM{6, 30, false}, true, 0, ClockHM{10, 42, false}, CalDate{7, 2, 2026});
  ui.onShort();                       // HOME -> MENU
  for (uint8_t i = 0; i < item; ++i) ui.onRotate(1, false);
  return ui;
}

int main() {
  printf("UIMenu tests\n");

  // --- HOME ---------------------------------------------------------------
  {
    UIMenu ui;
    ui.seed(ClockHM{6, 30, false}, true, 0, ClockHM{10, 42, false}, CalDate{7, 2, 2026});
    CHECK(ui.screen() == S::HOME, "boots on HOME");
    UIMenu::Result off = ui.onRotate(3, false);
    CHECK(off.cmd == C::NONE, "rotate on HOME with lamp OFF does nothing");
    UIMenu::Result on = ui.onRotate(2, true);
    CHECK(on.cmd == C::ADJUST_BRIGHTNESS && on.brightnessDelta == 2 * UI_BRIGHTNESS_STEP,
          "rotate on HOME with lamp ON adjusts brightness by delta*step");
    ui.onShort();
    CHECK(ui.screen() == S::MENU, "short on HOME opens MENU");
  }

  // --- MENU navigation + Alarm toggle + Exit ------------------------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_ALARM);
    ui.onRotate(-1, false);
    CHECK(ui.menuIndex() == UIMenu::MI_EXIT, "menu cursor wraps up to Exit");

    UIMenu tog = freshAtMenu(UIMenu::MI_ALARM_TOGGLE);
    bool wasArmed = tog.armed();
    UIMenu::Result r = tog.onShort();
    CHECK(r.cmd == C::SET_ARMED && r.armed == !wasArmed, "Alarm toggle emits SET_ARMED flipped");
    CHECK(tog.armed() == !wasArmed && tog.screen() == S::MENU, "toggle flips + stays in MENU");

    UIMenu ex = freshAtMenu(UIMenu::MI_EXIT);
    ex.onShort();
    CHECK(ex.screen() == S::HOME, "Exit returns to HOME");
  }

  // --- SET ALARM: the navigating/editing chain (spec SS 4.5) --------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_ALARM);
    ui.onShort();                      // enter Set Alarm
    CHECK(ui.screen() == S::SET_ALARM, "enter Set Alarm");
    CHECK(ui.cursor() == 0 && ui.mode() == M::NAVIGATING, "starts on HH, navigating (steady)");

    ui.onShort();
    CHECK(ui.cursor() == 0 && ui.mode() == M::EDITING, "1st short -> HH editing (flashing)");
    ui.onShort();
    CHECK(ui.cursor() == 1 && ui.mode() == M::EDITING, "short -> confirm HH, MM editing");
    ui.onShort();
    CHECK(ui.cursor() == 2 && ui.mode() == M::EDITING, "short -> AM/PM editing");
    ui.onShort();
    CHECK(ui.onCancelField() && ui.mode() == M::NAVIGATING, "short -> lands on [Cancel] navigating");
  }

  // --- SET ALARM: [Cancel] discards, committed value untouched ------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_ALARM);
    ui.onShort();                      // Set Alarm, HH navigating
    ui.onShort();                      // HH editing
    ui.onRotate(5, false);             // 6 -> 11
    CHECK(ui.alarmEdit().hour12 == 11, "editing HH: rotate changes the working hour");
    // advance to Cancel and confirm it
    ui.onShort(); ui.onShort(); ui.onShort();   // MM, AMPM, Cancel
    CHECK(ui.onCancelField(), "reached Cancel");
    ui.onShort();                      // confirm Cancel -> discard
    CHECK(ui.screen() == S::MENU, "Cancel exits to MENU");
    CHECK(ui.committedAlarm().hour12 == 6, "discard leaves committed alarm unchanged (still 6)");
  }

  // --- SET ALARM: long-press saves all ------------------------------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_ALARM);
    ui.onShort();                      // Set Alarm
    ui.onShort();                      // HH editing
    ui.onRotate(1, false);             // 6 -> 7
    UIMenu::Result r = ui.onLong();
    CHECK(r.cmd == C::COMMIT_ALARM && r.alarm.hour12 == 7, "long-press emits COMMIT_ALARM with edited value");
    CHECK(ui.committedAlarm().hour12 == 7 && ui.screen() == S::MENU, "save updates committed + exits");
  }

  // --- value wraps --------------------------------------------------------
  {
    UIMenu ui;                         // seed the committed alarm at 12:00 directly
    ui.seed(ClockHM{12, 0, false}, true, 0, ClockHM{12, 0, false}, CalDate{7, 2, 2026});
    ui.onShort();                      // HOME -> MENU (index 0 = Set Alarm)
    ui.onShort();                      // enter Set Alarm (working copy = 12:00)
    ui.onShort();                      // HH editing
    ui.onRotate(1, false);             // 12 -> 1
    CHECK(ui.alarmEdit().hour12 == 1, "HH 12 +1 wraps to 1");
    ui.onShort();                      // MM editing
    ui.onRotate(-1, false);            // 0 -> 59
    CHECK(ui.alarmEdit().minute == 59, "MM 0 -1 wraps to 59");
    ui.onShort();                      // AMPM editing
    bool pm0 = ui.alarmEdit().pm;
    ui.onRotate(1, false);
    CHECK(ui.alarmEdit().pm == !pm0, "AM/PM toggles");
  }

  // --- SET TIME: longer chain + year wrap ---------------------------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_TIME);
    ui.onShort();                      // enter Set Time (seeded from live clock)
    CHECK(ui.screen() == S::SET_TIME, "enter Set Time");
    // chain: HH MM AMPM MONTH DAY YEAR then Cancel (7 fields)
    ui.onShort();                      // HH editing
    for (int i = 0; i < 4; ++i) ui.onShort();   // -> MM AMPM MONTH DAY editing
    CHECK(ui.cursor() == 4 && ui.mode() == M::EDITING, "chained into DAY editing");
    ui.onShort();                      // YEAR editing (cursor 5)
    CHECK(ui.cursor() == 5 && ui.mode() == M::EDITING, "YEAR editing");
    ui.onRotate(-1, false);            // year 2026 -> 2025
    CHECK(ui.dateEdit().year == 2025, "editing YEAR changes it");
    ui.onShort();                      // -> Cancel navigating (cursor 6)
    CHECK(ui.onCancelField() && ui.mode() == M::NAVIGATING, "Set Time lands on Cancel after YEAR");

    UIMenu::Result r = ui.onLong();
    CHECK(r.cmd == C::COMMIT_TIME && r.date.year == 2025, "Set Time long-press emits COMMIT_TIME");
  }

  // --- COLOR PRESET: inverted grammar (spec SS 4.6) -----------------------
  {
    UIMenu ui;
    ui.seed(ClockHM{6, 30, false}, true, /*preset*/ 2, ClockHM{10, 42, false}, CalDate{7, 2, 2026});
    ui.onShort();                                   // MENU
    for (int i = 0; i < UIMenu::MI_COLOR_PRESET; ++i) ui.onRotate(1, false);
    ui.onShort();                                   // enter Color Preset
    CHECK(ui.screen() == S::COLOR_PRESET, "enter Color Preset");
    CHECK(ui.presetCursor() == 2, "picker starts on the committed preset");

    UIMenu::Result prev = ui.onRotate(1, false);
    CHECK(prev.cmd == C::PREVIEW_PRESET && prev.preset == 3, "rotate previews live (next preset)");

    UIMenu::Result cancel = ui.onShort();           // short = cancel/revert
    CHECK(cancel.cmd == C::PREVIEW_PRESET && cancel.preset == 2, "short reverts strip to entry preset");
    CHECK(ui.screen() == S::MENU, "cancel exits to MENU");

    // re-enter and save with long-press
    for (int i = 0; i < UIMenu::MI_COLOR_PRESET; ++i) {} // already on Color Preset item
    ui.onShort();                                   // enter Color Preset again
    ui.onRotate(2, false);                           // preview preset 4
    UIMenu::Result save = ui.onLong();
    CHECK(save.cmd == C::COMMIT_PRESET && save.preset == 4, "long-press commits the previewed preset");
  }

  // --- alarm fires mid-edit -> abort, nothing committed -------------------
  {
    UIMenu ui = freshAtMenu(UIMenu::MI_SET_ALARM);
    ui.onShort(); ui.onShort(); ui.onRotate(9, false);   // deep in an edit
    ui.abortToHome();
    CHECK(ui.screen() == S::HOME, "abortToHome returns to HOME");
    CHECK(ui.committedAlarm().hour12 == 6, "aborted edit commits nothing");
  }

  printf("\n%s (%d failure%s)\n", failures ? "FAILURES" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
