#pragma once
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "Pins.h"

// DFPlayer Mini (spec 3.6). Sends commands only; playback state comes from
// the BUSY pin, since serial replies can be corrupted by strip writes.
class DFPlayerDriver {
 public:
  void begin();

  // Starts, repeats or stops the wake song to match wantAudio.
  void update(bool wantAudio);

 private:
  SoftwareSerial      serial_{PIN_DF_TX, PIN_DF_RX};  // (rx, tx)
  DFRobotDFPlayerMini player_;
  bool     playing_ = false;
  uint32_t startedMs_ = 0;
};
