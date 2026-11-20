// AJ's Sunrise Alarm Clock (spec 4.9).
//
// Connects the pure-logic modules to the drivers. Each frame reads input,
// updates the logic, then refreshes the strip, OLED and audio. Nothing blocks.

#include "src/AlarmSequence.h"
#include "src/DFPlayerDriver.h"
#include "src/InputDriver.h"
#include "src/Lamp.h"
#include "src/OledDriver.h"
#include "src/Persistence.h"
#include "src/RtcDriver.h"
#include "src/StripDriver.h"
#include "src/UIMenu.h"

InputDriver    input;
RtcDriver      rtc;
Persistence    store;
StripDriver    strip;
OledDriver     oled;
DFPlayerDriver audio;

Lamp          lamp;
AlarmSequence alarm;
UIMenu        ui;

// Applies a menu result to the lamp, RTC or EEPROM.
void applyMenuResult(const UIMenu::Result& r) {
  switch (r.cmd) {
    case UIMenu::Cmd::ADJUST_BRIGHTNESS:
      lamp.adjustBrightness(r.brightnessDelta);
      store.setBrightness(lamp.brightness());
      break;
    case UIMenu::Cmd::SAVE_ALARM:  rtc.setAlarm(r.alarm); break;
    case UIMenu::Cmd::SAVE_TIME:   rtc.setClock(r.time, r.date); break;
    case UIMenu::Cmd::SET_ARMED:   store.setArmed(r.armed); break;
    case UIMenu::Cmd::SAVE_PRESET:
      lamp.setPreset(r.preset);
      store.setPreset(r.preset);
      break;
    case UIMenu::Cmd::NONE: break;
  }
}

// Soft power or a long press stops the alarm for today; a short press
// snoozes.
void handleAlarmInput(const InputEvents& in, uint32_t now, uint32_t today) {
  if (in.softPower || in.longPress) {
    alarm.stop();
    store.setRanOnDate(today);
    lamp.turnOff();
  } else if (in.shortPress) {
    alarm.snooze(now);
  }
}

// Routes input to the lamp and menu while no alarm is running.
void handleMenuInput(const InputEvents& in) {
  if (in.softPower) lamp.togglePower();
  bool onHome = (ui.screen() == UIMenu::Screen::HOME);
  if (in.rotation)   applyMenuResult(ui.onRotate(in.rotation, lamp.isOn() && onHome));
  if (in.shortPress) applyMenuResult(ui.onShort());
  if (in.longPress)  applyMenuResult(ui.onLong());
}

// Returns the alarm color, a full-brightness preset preview, or the lamp
// color, in that priority.
Rgbw stripColor(const AlarmOutput& out) {
  if (alarm.active()) return out.color;
  if (ui.screen() == UIMenu::Screen::COLOR_PRESET) return COLOR_PRESETS[ui.presetCursor()].color;
  return lamp.color();
}

void setup() {
  store.begin();
  strip.begin();
  oled.begin();
  input.begin();
  audio.begin();
  rtc.begin();  // runs on without a DS3231

  const PersistData& saved = store.data();
  lamp.setPreset(saved.preset);
  lamp.setBrightness(saved.brightness);
  ui.load(rtc.alarm(), saved.armed, saved.preset);

  // Power was cut during today's alarm: resume it (spec 4.8).
  bool ranToday = (saved.ranOnDate == rtc.todayKey());
  if (AlarmSequence::shouldResumeOnBoot(saved.armed, rtc.alarmTimePassedToday(), ranToday)) {
    alarm.resumeSounding();
  }
}

void loop() {
  const uint32_t now = millis();
  const uint32_t today = rtc.todayKey();
  InputEvents in = input.read();

  bool ranToday = (store.data().ranOnDate == today);
  if (AlarmSequence::shouldStart(rtc.alarmFired(), store.data().armed, ranToday, alarm.active())) {
    rtc.clearAlarmFired();
    alarm.start(now);
    ui.abortToHome();
  }

  if (alarm.active()) handleAlarmInput(in, now, today);
  else                handleMenuInput(in);

  AlarmOutput out = alarm.update(now);
  ui.setNow(rtc.clock(), rtc.date());

  strip.show(stripColor(out), now);
  oled.render(ui, DisplayStatus{lamp.isOn(), lamp.preset(), alarm.active(),
                                alarm.state() == AlarmSequence::SNOOZED,
                                out.snoozeRemainingMs},
              now, in.activity);
  audio.update(out.audioOn);
}
