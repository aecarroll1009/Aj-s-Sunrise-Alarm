// Bring-up step 3a (spec SS 3.8): I2C scan.
// Expect TWO devices: 0x3C (OLED) and 0x68 (DS3231). If either is missing,
// check SDA=A4 / SCL=A5 wiring and 5V/GND before going further.
#include <Wire.h>

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  Wire.begin();
  Serial.println(F("I2C scan (expect 0x3C OLED + 0x68 DS3231)"));
}

void loop() {
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  found 0x"));
      Serial.println(addr, HEX);
      ++found;
    }
  }
  Serial.print(F("total devices: "));
  Serial.println(found);
  Serial.println();
  delay(3000);
}
