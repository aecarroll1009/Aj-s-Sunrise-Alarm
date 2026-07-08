#include "Persistence.h"
#include <EEPROM.h>

namespace {
constexpr int     EE_ADDR = 0;
constexpr uint8_t MAGIC   = 0xA5;
constexpr uint8_t VERSION = 1;
}

void Persistence::begin() {
  EEPROM.get(EE_ADDR, d_);
  if (d_.magic != MAGIC || d_.version != VERSION) {
    d_.magic       = MAGIC;
    d_.version     = VERSION;
    d_.armed       = 1;
    d_.preset      = 0;
    d_.brightness  = DEFAULT_BRIGHTNESS;
    d_.firedOnDate = 0;
    save();
  }
}

void Persistence::save() {
  EEPROM.put(EE_ADDR, d_);   // put() uses update() internally -> wear-friendly
}

void Persistence::setArmed(bool a)        { d_.armed = a ? 1 : 0; save(); }
void Persistence::setPreset(uint8_t p)    { d_.preset = p; save(); }
void Persistence::setBrightness(uint8_t b){ d_.brightness = b; save(); }
void Persistence::setFired(uint32_t ymd)  { d_.firedOnDate = ymd; save(); }
