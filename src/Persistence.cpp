#include "Persistence.h"
#include <EEPROM.h>
#include "Tuning.h"

namespace {
constexpr int     EEPROM_ADDR = 0;
constexpr uint8_t MAGIC       = 0xA5;
constexpr uint8_t VERSION     = 1;
}  // namespace

void Persistence::begin() {
  EEPROM.get(EEPROM_ADDR, d_);
  if (d_.magic == MAGIC && d_.version == VERSION) return;
  d_ = PersistData{MAGIC, VERSION, 1, 0, DEFAULT_BRIGHTNESS, 0};
  save();
}

// put() skips unchanged bytes, saving EEPROM wear.
void Persistence::save() { EEPROM.put(EEPROM_ADDR, d_); }

void Persistence::setArmed(bool armed)         { d_.armed = armed ? 1 : 0; save(); }
void Persistence::setPreset(uint8_t preset)    { d_.preset = preset; save(); }
void Persistence::setBrightness(uint8_t b)     { d_.brightness = b; save(); }
void Persistence::setRanOnDate(uint32_t ymd)   { d_.ranOnDate = ymd; save(); }
