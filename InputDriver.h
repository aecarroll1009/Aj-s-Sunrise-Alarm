#pragma once
#include <stdint.h>
#include "Config.h"

// ============================================================================
// InputDriver — rotary encoder + encoder push + soft-power button (spec SS 3.3-3.4).
//
// Encoder CLK/DT (D2/D3) are decoded in an ISR with a full-step state table so
// mechanical bounce can't inject phantom counts; rotation accumulates as signed
// detents. The encoder push (D4) is classified into short vs long press, and the
// soft-power button (D5) into single presses -- both debounced, both
// INPUT_PULLUP / active-low. All events are latched and consumed via take*().
// ============================================================================

class InputDriver {
public:
  void begin();
  void poll();               // call every loop (buttons + timing)

  int16_t takeRotation();    // signed detents since last call (consumes)
  bool    takeEncShort();
  bool    takeEncLong();
  bool    takeSoftPower();
  bool    takeActivity();    // any input since last call (for OLED wake)

  // ISR trampolines (must be public/static for attachInterrupt).
  static void isrEncoder();

private:
  void onEncoderEdge();

  // --- encoder push (D4) debounce/classify ---
  bool     encDown_{false};
  uint32_t encDownMs_{0};
  bool     encLongFired_{false};
  bool     encLastRaw_{true};      // pulled-up idle = HIGH(true)
  uint32_t encEdgeMs_{0};

  // --- soft power (D5) ---
  bool     pwrLastRaw_{true};
  uint32_t pwrEdgeMs_{0};
  bool     pwrStable_{true};

  // --- latched events ---
  bool encShort_{false};
  bool encLong_{false};
  bool softPwr_{false};
  bool activity_{false};
};
