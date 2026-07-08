// Bring-up step 3b (spec SS 3.8): set + read the DS3231, and prove Alarm1 fires.
// Uploads the sketch build time into the RTC ONCE, then prints the time each
// second and programs Alarm1 for one minute ahead so you can watch the flag set.
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  if (!rtc.begin()) { Serial.println(F("DS3231 NOT FOUND")); while (1) {} }

  // Set the clock to this sketch's build time (comment out after the first run).
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);

  // Alarm1 one minute from now, matching hours+minutes+seconds.
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
