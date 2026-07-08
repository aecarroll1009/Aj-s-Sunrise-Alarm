#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// Persistence — EEPROM-backed non-volatile state (spec SS 4, fix 1).
//
// Nothing critical may live only in RAM, or a power blip forgets the alarm.
// The ALARM TIME itself lives in the DS3231 (battery-backed); this stores the
// rest: armed flag, firedOnDate (which day the alarm last fired -> dismissed),
// the color preset, and manual brightness. A magic+version guards a blank chip.
// EEPROM.put writes byte-by-byte via update(), so unchanged bytes don't wear.
// ============================================================================

struct PersistData {
  uint8_t  magic;        // MAGIC when valid
  uint8_t  version;      // layout version
  uint8_t  armed;        // 0/1
  uint8_t  preset;       // color preset index
  uint8_t  brightness;   // manual lamp brightness
  uint32_t firedOnDate;  // yyyymmdd of last fire (0 = none / re-armed)
};

class Persistence {
public:
  void begin();          // load; initialise defaults if the chip is blank/stale

  const PersistData& data() const { return d_; }

  void setArmed(bool a);
  void setPreset(uint8_t p);
  void setBrightness(uint8_t b);
  void setFired(uint32_t yyyymmdd);

private:
  void save();
  PersistData d_{};
};
