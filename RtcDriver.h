#pragma once
#include <RTClib.h>
#include "Config.h"

// ============================================================================
// RtcDriver — DS3231 wrapper (spec SS 3.5, 4, fix 2).
//
// The alarm TRIGGER is the DS3231 Alarm1 hardware flag, never now==alarmTime
// (an equality test can be skipped by a busy loop and never fire). The alarm
// time is programmed into Alarm1 in daily (hours:minutes:seconds) match mode,
// so it survives power loss on the CR2032. All times cross the API as 12-hour
// ClockHM; 24h conversion is internal.
// ============================================================================

class RtcDriver {
public:
  bool begin();                          // false if the DS3231 isn't found

  ClockHM  clock();                      // current time (12h)
  CalDate  date();                       // current date
  void     setClock(ClockHM t, CalDate d);

  void     setAlarm(ClockHM a);          // program Alarm1 to fire daily at a
  ClockHM  getAlarm();                   // read Alarm1 back (12h)

  bool     alarmFlagged();               // Alarm1 hardware flag set?
  void     clearAlarmFlag();             // clear it (once, at sequence start)
  bool     alarmPassedToday(ClockHM a);  // is now >= a (today)? (boot recovery)

  uint32_t todayKey();                   // yyyymmdd, for firedOnDate compares

private:
  RTC_DS3231 rtc_;
};
