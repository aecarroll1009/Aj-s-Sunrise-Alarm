// AJ's Sunrise Alarm Clock — main sketch (spec 4.9).
//
// Non-blocking state machine: no delay(); everything is timed off
// millis()/RTC. This file only wires the modules together; the behavior
// lives in the pure LightEngine, AlarmSequence, SunriseCurve, and UIMenu
// modules (all native-tested) and the thin hardware drivers. See
// docs/BRING_UP.md for the order to bring hardware up.

#include "src/Config.h"
#include "src/StripDriver.h"
#include "src/OledDriver.h"
#include "src/RtcDriver.h"
#include "src/DFPlayerDriver.h"
#include "src/InputDriver.h"
#include "src/Persistence.h"
#include "src/SunriseCurve.h"
#include "src/LightEngine.h"
#include "src/AlarmSequence.h"
#include "src/UIMenu.h"

// Hardware adapters.
StripDriver    strip;
OledDriver     oled;
RtcDriver      rtc;
DFPlayerDriver audio;
InputDriver    input;
Persistence    store;

// Pure-logic modules.
LightEngine    light;
AlarmSequence  alarm;
UIMenu         ui;

static uint32_t lastPreviewShowMs = 0;

// Input events latched since the previous frame.
struct InputEvents {
  int16_t rot;
  bool    encShort;
  bool    encLong;
  bool    soft;
  bool    activity;
};

// Polls the input driver and takes this frame's latched events.
//
// Returns:
//   The events that occurred since the last call.
static InputEvents pollInput() {
  input.poll();
  InputEvents e;
  e.rot      = input.takeRotation();
  e.encShort = input.takeEncShort();
  e.encLong  = input.takeEncLong();
  e.soft     = input.takeSoftPower();
  e.activity = input.takeActivity();
  return e;
}

// Applies a UI intent against the domain state and hardware.
//
// Args:
//   r: the intent produced by a UIMenu event handler.
static void applyUi(const UIMenu::Result& r) {
  switch (r.cmd) {
    case UIMenu::Cmd::ADJUST_BRIGHTNESS:
      light.adjustBrightness(r.brightnessDelta);
      store.setBrightness(light.brightness());  // update() only writes the changed byte
      break;
    case UIMenu::Cmd::COMMIT_ALARM:
      rtc.setAlarm(r.alarm);  // alarm time lives in the DS3231
      break;
    case UIMenu::Cmd::COMMIT_TIME:
      rtc.setClock(r.tod, r.date);
      break;
    case UIMenu::Cmd::SET_ARMED:
      store.setArmed(r.armed);
      break;
    case UIMenu::Cmd::PREVIEW_PRESET:  // live strip preview handled in updateStrip()
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

// Starts the wake sequence if the DS3231 hardware alarm flag says it's time
// (spec 4.9).
//
// Args:
//   now: current time.
//   today: current date key, from RtcDriver::todayKey().
static void checkAlarmTrigger(uint32_t now, uint32_t today) {
  bool dismissedToday = (store.data().firedOnDate == today);
  if (!AlarmSequence::shouldStartSunrise(rtc.alarmFlagged(), store.data().armed,
                                         dismissedToday, alarm.active())) {
    return;
  }
  rtc.clearAlarmFlag();
  alarm.startSunrise(now);
  light.beginOverride();
  ui.abortToHome();  // discard any uncommitted edit
}

// Routes input while the alarm is active. The alarm is authoritative until
// it's dismissed or cancelled.
//
// Args:
//   now: current time.
//   today: current date key, from RtcDriver::todayKey().
//   e: this frame's input events.
static void routeInputToAlarm(uint32_t now, uint32_t today, const InputEvents& e) {
  if (e.soft) {
    if (alarm.onSoftPower() == AlarmSequence::DISMISSED) {
      store.setFired(today);  // done for today
      light.endOverrideOff();
    }
  } else if (e.encLong) {
    if (alarm.onEncoderLong() == AlarmSequence::CANCELLED) {
      store.setFired(today);
      light.endOverrideOff();
    }
  } else if (e.encShort) {
    alarm.onEncoderShort(now);  // snoozes only while sounding
  }
}

// Routes input while the alarm is idle: the manual lamp and menu navigation.
//
// Args:
//   e: this frame's input events.
static void routeInputToMenu(const InputEvents& e) {
  if (e.soft) light.togglePower();
  bool lampOnHome = light.isOn() && (ui.screen() == UIMenu::Screen::HOME);
  if (e.rot)      applyUi(ui.onRotate(e.rot, lampOnHome));
  if (e.encShort) applyUi(ui.onShort());
  if (e.encLong)  applyUi(ui.onLong());
}

// Drives the strip from the alarm override or the manual lamp. The Color
// Preset screen previews live at a rate-limited refresh (spec 4.6/4.9).
//
// Args:
//   now: current time.
//   ao: this frame's alarm output.
static void updateStrip(uint32_t now, const AlarmOutput& ao) {
  if (ao.lightOverride) light.setOverrideColor(ao.lightColor);

  bool previewing = !alarm.active() && (ui.screen() == UIMenu::Screen::COLOR_PRESET);
  Rgbw stripColor = previewing ? COLOR_PRESETS[ui.presetCursor()].c : light.outputColor();
  strip.setAll(stripColor);
  if (previewing) {
    if ((now - lastPreviewShowMs) >= PREVIEW_MIN_MS) { strip.render(); lastPreviewShowMs = now; }
  } else {
    strip.render();
  }
}

// Refreshes the OLED with the current view-model and the alarm overlay.
//
// Args:
//   now: current time.
//   ao: this frame's alarm output.
//   activity: whether any input happened this frame.
static void updateDisplay(uint32_t now, const AlarmOutput& ao, bool activity) {
  ui.tick(rtc.clock(), rtc.date());
  const char* lampName = COLOR_PRESETS[light.presetIndex()].name;
  bool snoozing = (alarm.state() == AlarmSequence::SNOOZED);
  oled.render(ui, light.isOn(), lampName, alarm.active(), snoozing,
              ao.snoozeRemainingMs, now, activity);
}

void setup() {
  store.begin();
  strip.begin();
  oled.begin();
  input.begin();
  audio.begin();
  rtc.begin();  // proceed even if absent; the bring-up sketch verifies the DS3231

  const PersistData& p = store.data();
  light.restore(p.preset, p.brightness);
  ui.seed(rtc.getAlarm(), p.armed, p.preset, rtc.clock(), rtc.date());

  // Power-loss recovery (spec 4.8): if the alarm time already passed today
  // and it was never dismissed, come back up sounding rather than silently
  // skipping it.
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

  InputEvents e = pollInput();

  checkAlarmTrigger(now, today);

  if (alarm.active()) routeInputToAlarm(now, today, e);
  else                routeInputToMenu(e);

  AlarmOutput ao = alarm.update(now);
  updateStrip(now, ao);
  updateDisplay(now, ao, e.activity);

  audio.update(ao.audioOn);  // write-only; reconciles via the BUSY pin
}
