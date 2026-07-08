#include "InputDriver.h"
#include <Arduino.h>

// ---- encoder decode (Ben Buxton full-step state table) ---------------------
namespace {
constexpr uint8_t R_START     = 0x0;
constexpr uint8_t R_CW_FINAL  = 0x1;
constexpr uint8_t R_CW_BEGIN  = 0x2;
constexpr uint8_t R_CW_NEXT   = 0x3;
constexpr uint8_t R_CCW_BEGIN = 0x4;
constexpr uint8_t R_CCW_FINAL = 0x5;
constexpr uint8_t R_CCW_NEXT  = 0x6;
constexpr uint8_t DIR_CW      = 0x10;
constexpr uint8_t DIR_CCW     = 0x20;

const uint8_t TTABLE[7][4] = {
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

volatile uint8_t g_state = R_START;
volatile int16_t g_accum = 0;         // signed detents, drained by takeRotation()
InputDriver*     g_self  = nullptr;
}

void InputDriver::isrEncoder() {
  if (g_self) g_self->onEncoderEdge();
}

void InputDriver::onEncoderEdge() {
  uint8_t pinstate = (uint8_t)((digitalRead(PIN_ENC_DT) << 1) | digitalRead(PIN_ENC_CLK));
  g_state = TTABLE[g_state & 0x0f][pinstate];
  uint8_t dir = g_state & 0x30;
  if (dir == DIR_CW)  ++g_accum;
  else if (dir == DIR_CCW) --g_accum;
}

void InputDriver::begin() {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW,  INPUT_PULLUP);
  pinMode(PIN_SOFT_PWR, INPUT_PULLUP);
  g_self = this;
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT),  isrEncoder, CHANGE);
}

int16_t InputDriver::takeRotation() {
  noInterrupts();
  int16_t v = g_accum;
  g_accum = 0;
  interrupts();
  if (v != 0) activity_ = true;
  return v;
}

void InputDriver::poll() {
  const uint32_t now = millis();

  // --- encoder push (D4), debounced, short vs long ---
  bool encRaw = (digitalRead(PIN_ENC_SW) == LOW);   // pressed = LOW
  if (encRaw != encLastRaw_) { encLastRaw_ = encRaw; encEdgeMs_ = now; }
  if ((now - encEdgeMs_) >= DEBOUNCE_MS) {
    if (encRaw && !encDown_) {                       // debounced press
      encDown_ = true; encDownMs_ = now; encLongFired_ = false; activity_ = true;
    } else if (!encRaw && encDown_) {                // debounced release
      encDown_ = false;
      if (!encLongFired_) encShort_ = true;          // released before long threshold
    } else if (encRaw && encDown_ && !encLongFired_ &&
               (now - encDownMs_) >= LONG_PRESS_MS) {
      encLong_ = true; encLongFired_ = true;         // fire long once while still held
    }
  }

  // --- soft power (D5), debounced single press on the press edge ---
  bool pwrRaw = (digitalRead(PIN_SOFT_PWR) == LOW);
  if (pwrRaw != pwrLastRaw_) { pwrLastRaw_ = pwrRaw; pwrEdgeMs_ = now; }
  if ((now - pwrEdgeMs_) >= DEBOUNCE_MS && pwrRaw != pwrStable_) {
    pwrStable_ = pwrRaw;
    if (pwrRaw) { softPwr_ = true; activity_ = true; }  // register on press
  }
}

bool InputDriver::takeEncShort()   { bool v = encShort_; encShort_ = false; return v; }
bool InputDriver::takeEncLong()    { bool v = encLong_;  encLong_  = false; return v; }
bool InputDriver::takeSoftPower()  { bool v = softPwr_;  softPwr_  = false; return v; }
bool InputDriver::takeActivity()   { bool v = activity_; activity_ = false; return v; }
