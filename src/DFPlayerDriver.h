#pragma once
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "Config.h"

// Audio wrapper (spec 3.6, fix 4).
//
// This driver is write-only: it never polls the DFPlayer's serial line,
// since reading it can collide with the LED strip's interrupt masking and
// corrupt SoftwareSerial. Playback status instead comes from the BUSY pin
// (D9, low while playing). update() reconciles a desired audio level into
// play, stop, or replay commands.
class DFPlayerDriver {
public:
  void begin();

  // Reconciles the desired audio level: starts playback on a rising want,
  // replays the track once it ends while still wanted, and stops it on a
  // falling want.
  //
  // Args:
  //   wantAudio: whether audio should be playing right now.
  void update(bool wantAudio);

private:
  // SoftwareSerial(rxPin, txPin): rx = D8 (from DFPlayer TX), tx = D7 (to DFPlayer RX)
  SoftwareSerial      ss_{PIN_DF_TX, PIN_DF_RX};
  DFRobotDFPlayerMini df_;
  bool     playing_{false};
  uint32_t lastStartMs_{0};
};
