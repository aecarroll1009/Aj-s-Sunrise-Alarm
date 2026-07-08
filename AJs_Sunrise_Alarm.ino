// ============================================================================
// AJ's Sunrise Alarm Clock — main sketch (spec SS 4.9).
//
// Non-blocking state machine: no delay(); everything is timed off millis()/RTC.
// This file only wires the modules together; the behavior lives in the pure
// LightEngine / AlarmSequence / SunriseCurve / UIMenu (native-tested) and the
// thin hardware drivers. See BRING_UP.md for the order to bring hardware up.
// ============================================================================

#include "Config.h"
#include "StripDriver.h"
#include "OledDriver.h"
#include "RtcDriver.h"
#include "DFPlayerDriver.h"
#include "InputDriver.h"
#include "Persistence.h"
#include "SunriseCurve.h"
#include "LightEngine.h"
#include "AlarmSequence.h"
#include "UIMenu.h"

// --- hardware adapters ---
StripDriver    strip;
OledDriver     oled;
RtcDriver      rtc;
DFPlayerDriver audio;
InputDriver    input;
Persistence    store;

// --- pure-logic modules ---
LightEngine    light;
AlarmSequence  alarm;
UIMenu         ui;

static uint32_t lastPreviewShowMs = 0;

// Apply a UI intent against the domain/hardware.
static void applyUi(const UIMenu::Result& r) {
  switch (r.cmd) {
    case UIMenu::Cmd::ADJUST_BRIGHTNESS:
      light.adjustBrightness(r.brightnessDelta);
      store.setBrightness(light.brightness());   // update() only writes the changed byte
      break;
    case UIMenu::Cmd::COMMIT_ALARM:
      rtc.setAlarm(r.alarm);                      // alarm time lives in the DS3231
      break;
    case UIMenu::Cmd::COMMIT_TIME:
      rtc.setClock(r.tod, r.date);
      break;
    case UIMenu::Cmd::SET_ARMED:
      store.setArmed(r.armed);
      break;
    case UIMenu::Cmd::PREVIEW_PRESET:             // live strip preview handled in loop()
      light.setPreset(r.preset);
      break;
    case UIMenu::Cmd::COMMIT_PRESET:
      light.setPreset(r.preset);
      store.setPreset(r.preset);
      break;
    case UIMenu::Cmd::NONE:
      break;
  }
}

void setup() {
  store.begin();
  strip.begin();
  oled.begin();
  input.begin();
  audio.begin();
  rtc.begin();   // proceed even if absent; bring-up sketch verifies the DS3231

  const PersistData& p = store.data();
  light.restore(p.preset, p.brightness);
  ui.seed(rtc.getAlarm(), p.armed, p.preset, rtc.clock(), rtc.date());

  // Power-loss recovery (spec SS 4.8): if the alarm time already passed today and
  // it was never dismissed, come up SOUNDING rather than silently skipping.
  bool dismissedToday = (p.firedOnDate == rtc.todayKey());
  if (AlarmSequence::shouldResumeOnBoot(p.armed, rtc.alarmPassedToday(rtc.getAlarm()),
                                        dismissedToday)) {
    alarm.resumeSounding(millis());
    light.beginOverride();
  }
}

void loop() {
  const uint32_t now = millis();
  const uint32_t today = rtc.todayKey();

  input.poll();
  int16_t rot      = input.takeRotation();
  bool    encShort = input.takeEncShort();
  bool    encLong  = input.takeEncLong();
  bool    soft     = input.takeSoftPower();
  bool    activity = input.takeActivity();

  // --- trigger: fire on the DS3231 hardware flag (spec SS 4.9) ---
  bool dismissedToday = (store.data().firedOnDate == today);
  if (AlarmSequence::shouldStartSunrise(rtc.alarmFlagged(), store.data().armed,
                                        dismissedToday, alarm.active())) {
    rtc.clearAlarmFlag();
    alarm.startSunrise(now);
    light.beginOverride();
    ui.abortToHome();                 // discard any uncommitted edit
  }

  // --- route input: the alarm is authoritative once active ---
  if (alarm.active()) {
    if (soft) {
      if (alarm.onSoftPower() == AlarmSequence::DISMISSED) {
        store.setFired(today);        // done for today
        light.endOverrideOff();
      }
    } else if (encLong) {
      if (alarm.onEncoderLong() == AlarmSequence::CANCELLED) {
        store.setFired(today);
        light.endOverrideOff();
      }
    } else if (encShort) {
      alarm.onEncoderShort(now);      // snooze (only while sounding)
    }
  } else {
    if (soft) light.togglePower();
    bool lampOnHome = light.isOn() && (ui.screen() == UIMenu::Screen::HOME);
    if (rot)      applyUi(ui.onRotate(rot, lampOnHome));
    if (encShort) applyUi(ui.onShort());
    if (encLong)  applyUi(ui.onLong());
  }

  // --- light: alarm override or manual ---
  AlarmOutput ao = alarm.update(now);
  if (ao.lightOverride) light.setOverrideColor(ao.lightColor);

  bool previewing = !alarm.active() && (ui.screen() == UIMenu::Screen::COLOR_PRESET);
  Rgbw stripColor = previewing ? COLOR_PRESETS[ui.presetCursor()].c : light.outputColor();
  strip.setAll(stripColor);
  if (previewing) {                   // rate-limit preview show() (spec SS 4.6/4.9)
    if ((now - lastPreviewShowMs) >= PREVIEW_MIN_MS) { strip.render(); lastPreviewShowMs = now; }
  } else {
    strip.render();
  }

  // --- display ---
  ui.tick(rtc.clock(), rtc.date());
  const char* lampName = COLOR_PRESETS[light.presetIndex()].name;
  bool snoozing = (alarm.state() == AlarmSequence::SNOOZED);
  oled.render(ui, light.isOn(), lampName, alarm.active(), snoozing,
              ao.snoozeRemainingMs, now, activity);

  // --- audio (write-only; reconciles via the BUSY pin) ---
  audio.update(ao.audioOn);
}
