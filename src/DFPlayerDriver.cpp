#include "DFPlayerDriver.h"
#include <Arduino.h>

namespace {
constexpr uint32_t MIN_PLAY_MS = 800;  // ignore BUSY for this long after starting a track
} // namespace

void DFPlayerDriver::begin() {
  pinMode(PIN_DF_BUSY, INPUT_PULLUP);
  ss_.begin(9600);
  // ack=false means fire-and-forget (no serial reads); doReset=true means a clean init.
  df_.begin(ss_, /*isACK=*/false, /*doReset=*/true);
  df_.volume(ALARM_VOLUME);  // 0..30
  playing_ = false;
}

void DFPlayerDriver::update(bool wantAudio) {
  const uint32_t now = millis();
  const bool busy = (digitalRead(PIN_DF_BUSY) == LOW);  // low means a track is playing

  if (wantAudio) {
    if (!playing_) {
      df_.volume(ALARM_VOLUME);
      df_.play(ALARM_TRACK);  // /mp3/0001.mp3
      playing_     = true;
      lastStartMs_ = now;
    } else if ((now - lastStartMs_) > MIN_PLAY_MS && !busy) {
      df_.play(ALARM_TRACK);  // track ended but still wanted, so replay it
      lastStartMs_ = now;
    }
  } else if (playing_) {
    df_.stop();
    playing_ = false;
  }
}
