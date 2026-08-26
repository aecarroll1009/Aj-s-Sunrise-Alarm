#pragma once
#include <RTClib.h>
#include "Config.h"

// DS3231 wrapper (spec 3.5, fix 2).
//
// The alarm trigger is the DS3231 Alarm1 hardware flag, never a
// now == alarmTime comparison, since an equality check can be skipped by a
// busy loop and never fire. The alarm time is programmed into Alarm1 in
// daily hours:minutes:seconds match mode, so it survives power loss on the
// CR2032. All times cross this API as 12-hour ClockHM values; the 24-hour
// conversion is internal.
class RtcDriver {
public:
  // Initializes the DS3231 and disables its unused square-wave and second
  // alarm.
  //
  // Returns:
  //   False if the DS3231 wasn't found on the bus.
  bool begin();

  // Returns the current time of day, in 12-hour form.
  ClockHM clock();

  // Returns the current calendar date.
  CalDate date();

  // Sets the RTC's date and time.
  //
  // Args:
  //   t: time of day to set.
  //   d: calendar date to set.
  void setClock(ClockHM t, CalDate d);

  // Programs Alarm1 to fire daily at a given time.
  //
  // Args:
  //   a: time of day the alarm should fire.
  void setAlarm(ClockHM a);

  // Returns the time Alarm1 is currently programmed for, in 12-hour form.
  ClockHM getAlarm();

  // Returns whether the Alarm1 hardware flag is set.
  bool alarmFlagged();

  // Clears the Alarm1 hardware flag. Call once, at the start of a sequence.
  void clearAlarmFlag();

  // Checks whether a given alarm time has already passed today, for boot
  // recovery.
  //
  // Args:
  //   a: alarm time to check.
  // Returns:
  //   True if the current time of day is at or past a.
  bool alarmPassedToday(ClockHM a);

  // Returns the current date as a yyyymmdd integer, for firedOnDate compares.
  uint32_t todayKey();

private:
  RTC_DS3231 rtc_;
};
