#pragma once
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "Config.h"

// ============================================================================
// DFPlayerDriver — audio wrapper (spec SS 3.6, 4, fix 4).
//
// WRITE-ONLY: we never poll the DFPlayer's serial (reading it can collide with
// the LED strip's interrupt masking and corrupt SoftwareSerial). Playback
// status comes from the BUSY pin (D9, LOW = playing). update() reconciles a
// desired audio level into play/stop/replay commands.
// ============================================================================

class DFPlayerDriver {
public:
  void begin();
  void update(bool wantAudio);   // start on rising want, replay on song-end, stop on falling

private:
  // SoftwareSerial(rxPin, txPin): rx = D8 (from DFPlayer TX), tx = D7 (to DFPlayer RX)
  SoftwareSerial      ss_{PIN_DF_TX, PIN_DF_RX};
  DFRobotDFPlayerMini df_;
  bool     playing_{false};
  uint32_t lastStartMs_{0};
};
