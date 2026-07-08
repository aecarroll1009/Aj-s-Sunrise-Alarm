#include "RtcDriver.h"

namespace {
// 12h <-> 24h helpers. 12 AM -> 0, 12 PM -> 12.
uint8_t to24(ClockHM t) {
  uint8_t h = t.hour12 % 12;          // 12 -> 0
  return t.pm ? h + 12 : h;
}
ClockHM from24(uint8_t h24, uint8_t minute) {
  ClockHM t;
  t.pm     = (h24 >= 12);
  uint8_t h = h24 % 12;
  t.hour12 = (h == 0) ? 12 : h;
  t.minute = minute;
  return t;
}
}

bool RtcDriver::begin() {
  if (!rtc_.begin()) return false;
  rtc_.disable32K();
  rtc_.writeSqwPinMode(DS3231_OFF);   // INT/SQW used for the alarm, not a square wave
  rtc_.disableAlarm(2);
  rtc_.clearAlarm(1);
  rtc_.clearAlarm(2);
  return true;
}

ClockHM RtcDriver::clock() {
  DateTime n = rtc_.now();
  return from24(n.hour(), n.minute());
}

CalDate RtcDriver::date() {
  DateTime n = rtc_.now();
  return CalDate{ (uint8_t)n.month(), (uint8_t)n.day(), (uint16_t)n.year() };
}

void RtcDriver::setClock(ClockHM t, CalDate d) {
  rtc_.adjust(DateTime(d.year, d.month, d.day, to24(t), t.minute, 0));
}

void RtcDriver::setAlarm(ClockHM a) {
  // Daily match on hours+minutes+seconds; the date fields are ignored in this mode.
  rtc_.clearAlarm(1);
  rtc_.setAlarm1(DateTime(2000, 1, 1, to24(a), a.minute, 0), DS3231_A1_Hour);
}

ClockHM RtcDriver::getAlarm() {
  DateTime a = rtc_.getAlarm1();
  return from24(a.hour(), a.minute());
}

bool RtcDriver::alarmFlagged()   { return rtc_.alarmFired(1); }
void RtcDriver::clearAlarmFlag() { rtc_.clearAlarm(1); }

bool RtcDriver::alarmPassedToday(ClockHM a) {
  DateTime n = rtc_.now();
  int nowMin = n.hour() * 60 + n.minute();
  int almMin = to24(a) * 60 + a.minute;
  return nowMin >= almMin;
}

uint32_t RtcDriver::todayKey() {
  DateTime n = rtc_.now();
  return (uint32_t)n.year() * 10000UL + (uint32_t)n.month() * 100UL + n.day();
}
