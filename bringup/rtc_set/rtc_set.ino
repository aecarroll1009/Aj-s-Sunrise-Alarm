// Bring-up step 3b: set the DS3231 clock and check that Alarm1 fires.
//
// Sets the clock to the build time, prints the time each second, and sets
// Alarm1 one minute ahead.
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  if (!rtc.begin()) { Serial.println(F("DS3231 NOT FOUND")); while (1) {} }

  // Comment out after the first run, or every reset rewinds the clock.
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);

  // Alarm1 one minute from now.
  DateTime t = rtc.now() + TimeSpan(60);
  rtc.setAlarm1(t, DS3231_A1_Hour);
  Serial.print(F("Alarm1 set for ")); Serial.print(t.hour());
  Serial.print(':'); Serial.println(t.minute());
}

void loop() {
  DateTime n = rtc.now();
  Serial.print(n.year()); Serial.print('-'); Serial.print(n.month());
  Serial.print('-'); Serial.print(n.day()); Serial.print(' ');
  Serial.print(n.hour()); Serial.print(':'); Serial.print(n.minute());
  Serial.print(':'); Serial.print(n.second());
  if (rtc.alarmFired(1)) { Serial.print(F("  <-- ALARM1 FIRED")); rtc.clearAlarm(1); }
  Serial.println();
  delay(1000);
}
