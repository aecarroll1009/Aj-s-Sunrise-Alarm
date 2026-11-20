#include "DFPlayerDriver.h"
#include <Arduino.h>
#include "Tuning.h"

namespace {
constexpr uint32_t BUSY_SETTLE_MS = 800;  // BUSY lags a play command
}  // namespace

void DFPlayerDriver::begin() {
  pinMode(PIN_DF_BUSY, INPUT_PULLUP);
  serial_.begin(9600);
  player_.begin(serial_, /*isACK=*/false, /*doReset=*/true);
  player_.volume(ALARM_VOLUME);
}

void DFPlayerDriver::update(bool wantAudio) {
  const uint32_t now = millis();

  if (!wantAudio) {
    if (playing_) player_.stop();
    playing_ = false;
    return;
  }

  bool trackEnded = playing_ && now - startedMs_ > BUSY_SETTLE_MS &&
                    digitalRead(PIN_DF_BUSY) == HIGH;
  if (!playing_ || trackEnded) {
    player_.volume(ALARM_VOLUME);
    player_.playMp3Folder(ALARM_TRACK);  // /mp3/0001.mp3
    playing_ = true;
    startedMs_ = now;
  }
}
