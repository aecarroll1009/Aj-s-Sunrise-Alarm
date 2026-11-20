#pragma once
#include <RTClib.h>
#include "Types.h"

// DS3231 clock and alarm (spec 3.5).
//
// The alarm time lives in Alarm1, matching daily, so it survives power cuts.
// The alarm triggers from Alarm1's hardware flag, which a slow loop can't
// miss the way a time comparison can.
class RtcDriver {
 public:
  // Starts the DS3231 and disables its unused outputs.
  //
  // Returns:
  //   False if the DS3231 isn't on the bus.
  bool begin();

  ClockHM clock();
  CalDate date();
  void setClock(ClockHM t, CalDate d);

  // Sets Alarm1 to fire every day at a.
  void setAlarm(ClockHM a);
  ClockHM alarm();

  // Returns whether Alarm1 fired since the flag was last cleared.
  bool alarmFired();
  void clearAlarmFired();

  bool alarmTimePassedToday();

  // Returns today as yyyymmdd.
  uint32_t todayKey();

 private:
  RTC_DS3231 rtc_;
};
