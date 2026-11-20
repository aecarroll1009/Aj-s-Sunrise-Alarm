#pragma once
#include <stdint.h>

// Settings kept in EEPROM across power cuts (spec 4). The alarm time lives
// in the DS3231 instead.
struct PersistData {
  uint8_t  magic;       // detects a blank chip
  uint8_t  version;     // detects an old layout
  uint8_t  armed;
  uint8_t  preset;
  uint8_t  brightness;
  uint32_t ranOnDate;   // yyyymmdd the alarm last ran; 0 if never
};

class Persistence {
 public:
  // Loads settings, writing defaults if the EEPROM is blank or outdated.
  void begin();

  const PersistData& data() const { return d_; }

  void setArmed(bool armed);
  void setPreset(uint8_t preset);
  void setBrightness(uint8_t brightness);
  void setRanOnDate(uint32_t yyyymmdd);

 private:
  void save();
  PersistData d_{};
};
