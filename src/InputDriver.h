#pragma once
#include <stdint.h>
#include "Config.h"

// Rotary encoder, encoder push, and soft-power button (spec 3.3-3.4).
//
// Encoder CLK/DT (D2/D3) are decoded in an ISR with a full-step state table
// so mechanical bounce can't inject phantom counts; rotation accumulates as
// signed detents. The encoder push (D4) is classified into short versus
// long presses, and the soft-power button (D5) into single presses; both
// are debounced, and both are INPUT_PULLUP and active-low. All events are
// latched and consumed through the take*() methods.
class InputDriver {
public:
  void begin();

  // Polls both buttons for debouncing and press classification. Call this
  // once per loop iteration.
  void poll();

  // Returns the signed detent count accumulated since the last call, then
  // resets it to zero.
  int16_t takeRotation();

  bool takeEncShort();
  bool takeEncLong();
  bool takeSoftPower();

  // Returns whether any input happened since the last call, for OLED wake.
  bool takeActivity();

  // ISR trampoline; must be public and static for attachInterrupt.
  static void isrEncoder();

private:
  void onEncoderEdge();

  // Debounces and classifies the encoder push button (D4).
  //
  // Args:
  //   now: current time, from millis().
  void pollEncoderButton(uint32_t now);

  // Debounces the soft-power button (D5).
  //
  // Args:
  //   now: current time, from millis().
  void pollSoftPower(uint32_t now);

  // Encoder push (D4) debounce and classify state.
  bool     encDown_{false};
  uint32_t encDownMs_{0};
  bool     encLongFired_{false};
  bool     encLastRaw_{true};  // pulled-up idle is high (true)
  uint32_t encEdgeMs_{0};

  // Soft power (D5) state.
  bool     pwrLastRaw_{true};
  uint32_t pwrEdgeMs_{0};
  bool     pwrStable_{true};

  // Latched events, cleared by their take*() method.
  bool encShort_{false};
  bool encLong_{false};
  bool softPwr_{false};
  bool activity_{false};
};
