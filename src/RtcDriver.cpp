#include "RtcDriver.h"

namespace {

// Returns the 24-hour hour of t (12 AM is 0).
uint8_t hour24(ClockHM t) {
  uint8_t h = t.hour12 % 12;
  return t.pm ? h + 12 : h;
}

// Converts a 24-hour time to 12-hour.
ClockHM from24(uint8_t h24, uint8_t minute) {
  uint8_t h = h24 % 12;
  return ClockHM{(uint8_t)(h == 0 ? 12 : h), minute, h24 >= 12};
}

}  // namespace

bool RtcDriver::begin() {
  if (!rtc_.begin()) return false;
  rtc_.disable32K();
  rtc_.writeSqwPinMode(DS3231_OFF);
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
  return CalDate{(uint8_t)n.month(), (uint8_t)n.day(), (uint16_t)n.year()};
}

void RtcDriver::setClock(ClockHM t, CalDate d) {
  rtc_.adjust(DateTime(d.year, d.month, d.day, hour24(t), t.minute, 0));
}

void RtcDriver::setAlarm(ClockHM a) {
  // Hour-match mode ignores the date.
  rtc_.clearAlarm(1);
  rtc_.setAlarm1(DateTime(2000, 1, 1, hour24(a), a.minute, 0), DS3231_A1_Hour);
}

ClockHM RtcDriver::alarm() {
  DateTime a = rtc_.getAlarm1();
  return from24(a.hour(), a.minute());
}

bool RtcDriver::alarmFired()      { return rtc_.alarmFired(1); }
void RtcDriver::clearAlarmFired() { rtc_.clearAlarm(1); }

bool RtcDriver::alarmTimePassedToday() {
  DateTime n = rtc_.now();
  DateTime a = rtc_.getAlarm1();
  return n.hour() * 60 + n.minute() >= a.hour() * 60 + a.minute();
}

uint32_t RtcDriver::todayKey() {
  DateTime n = rtc_.now();
  return (uint32_t)n.year() * 10000UL + (uint32_t)n.month() * 100UL + n.day();
}
