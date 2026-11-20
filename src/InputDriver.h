#pragma once
#include <stdint.h>
#include "Pins.h"

// Rotary encoder, its push button, and the soft-power button (spec 3.3-3.4).

// Input since the previous read().
struct InputEvents {
  int16_t rotation;    // signed detents
  bool    shortPress;  // encoder button
  bool    longPress;   // encoder button
  bool    softPower;
  bool    activity;    // any input, for waking the OLED
};

// A debounced, active-low push button.
class Button {
 public:
  struct Events {
    bool pressed;     // went down
    bool shortPress;  // released before a long press
    bool longPress;   // fires once per hold
  };

  explicit Button(uint8_t pin) : pin_(pin) {}
  void begin();

  // Returns what changed since the last call. Call once per frame.
  Events poll(uint32_t nowMs);

 private:
  uint8_t  pin_;
  bool     lastRaw_ = false;
  uint32_t lastEdgeMs_ = 0;
  bool     down_ = false;
  uint32_t downMs_ = 0;
  bool     longSent_ = false;
};

// Reads all inputs. The encoder is decoded in an interrupt; the buttons
// are polled.
class InputDriver {
 public:
  void begin();

  // Returns input since the last call. Call once per frame.
  InputEvents read();

 private:
  Button encoderButton_{PIN_ENC_SW};
  Button powerButton_{PIN_SOFT_PWR};
};
