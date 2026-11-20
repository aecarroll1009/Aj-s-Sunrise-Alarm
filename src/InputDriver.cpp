#include "InputDriver.h"
#include <Arduino.h>

namespace {

constexpr uint8_t  DEBOUNCE_MS   = 10;
constexpr uint16_t LONG_PRESS_MS = 600;

// Ben Buxton's full-step encoder table. Contact bounce never completes a
// step, so it can't add phantom detents.
constexpr uint8_t R_START     = 0x0;
constexpr uint8_t R_CW_FINAL  = 0x1;
constexpr uint8_t R_CW_BEGIN  = 0x2;
constexpr uint8_t R_CW_NEXT   = 0x3;
constexpr uint8_t R_CCW_BEGIN = 0x4;
constexpr uint8_t R_CCW_FINAL = 0x5;
constexpr uint8_t R_CCW_NEXT  = 0x6;
constexpr uint8_t DIR_CW      = 0x10;
constexpr uint8_t DIR_CCW     = 0x20;

const uint8_t STATE_TABLE[7][4] = {
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

volatile uint8_t encoderState = R_START;
volatile int16_t detents = 0;

// Interrupt handler for CLK and DT edges.
void onEncoderEdge() {
  uint8_t pins = (uint8_t)((digitalRead(PIN_ENC_DT) << 1) | digitalRead(PIN_ENC_CLK));
  encoderState = STATE_TABLE[encoderState & 0x0f][pins];
  uint8_t dir = encoderState & 0x30;
  if (dir == DIR_CW) ++detents;
  else if (dir == DIR_CCW) --detents;
}

// Returns detents since the last call and resets the count.
int16_t takeDetents() {
  noInterrupts();
  int16_t n = detents;
  detents = 0;
  interrupts();
  return n;
}

}  // namespace

void Button::begin() { pinMode(pin_, INPUT_PULLUP); }

Button::Events Button::poll(uint32_t nowMs) {
  Events e{false, false, false};
  bool raw = (digitalRead(pin_) == LOW);
  if (raw != lastRaw_) {
    lastRaw_ = raw;
    lastEdgeMs_ = nowMs;
  }
  if (nowMs - lastEdgeMs_ < DEBOUNCE_MS) return e;

  if (raw && !down_) {
    down_ = true;
    downMs_ = nowMs;
    longSent_ = false;
    e.pressed = true;
  } else if (!raw && down_) {
    down_ = false;
    e.shortPress = !longSent_;
  } else if (down_ && !longSent_ && nowMs - downMs_ >= LONG_PRESS_MS) {
    longSent_ = true;
    e.longPress = true;
  }
  return e;
}

void InputDriver::begin() {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  encoderButton_.begin();
  powerButton_.begin();
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), onEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), onEncoderEdge, CHANGE);
}

InputEvents InputDriver::read() {
  const uint32_t now = millis();
  Button::Events enc = encoderButton_.poll(now);
  Button::Events pwr = powerButton_.poll(now);

  InputEvents e;
  e.rotation   = takeDetents();
  e.shortPress = enc.shortPress;
  e.longPress  = enc.longPress;
  e.softPower  = pwr.pressed;
  e.activity   = e.rotation != 0 || enc.pressed || enc.longPress || e.softPower;
  return e;
}
