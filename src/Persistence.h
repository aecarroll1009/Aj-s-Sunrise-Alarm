#pragma once
#include <stdint.h>
#include "Config.h"

// EEPROM-backed non-volatile state (spec 4, fix 1).
//
// Nothing critical may live only in RAM, or a power blip forgets the alarm.
// The alarm time itself lives in the DS3231 (battery-backed); this stores
// the rest: the armed flag, firedOnDate (which day the alarm last fired, so
// it's known to be dismissed), the color preset, and manual brightness. A
// magic byte plus a version guard against a blank chip. EEPROM.put() writes
// byte by byte through update(), so unchanged bytes don't wear the chip.
struct PersistData {
  uint8_t  magic;        // MAGIC when valid
  uint8_t  version;      // layout version
  uint8_t  armed;        // 0 or 1
  uint8_t  preset;       // color preset index
  uint8_t  brightness;   // manual lamp brightness
  uint32_t firedOnDate;  // yyyymmdd of last fire (0 means none, or re-armed)
};

class Persistence {
public:
  // Loads saved state, initializing defaults if the chip is blank or the
  // layout version doesn't match.
  void begin();

  const PersistData& data() const { return d_; }

  void setArmed(bool a);
  void setPreset(uint8_t p);
  void setBrightness(uint8_t b);
  void setFired(uint32_t yyyymmdd);

private:
  void save();
  PersistData d_{};
};
