// Bring-up step 4 (spec SS 3.8): rotary encoder + encoder push + soft-power button.
// Prints a running detent count and press events. Turning CW should count up; if
// it counts down, swap CLK/DT (or just flip the sign in InputDriver later).
//   CLK=D2  DT=D3  SW=D4  SoftPower=D5   (all INPUT_PULLUP)

constexpr uint8_t PIN_CLK = 2, PIN_DT = 3, PIN_SW = 4, PIN_PWR = 5;

volatile int16_t g_pos = 0;
volatile uint8_t g_state = 0;
const uint8_t TT[7][4] = {
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10}, {0x3, 0x2, 0x0, 0x0},
  {0x3, 0x2, 0x1, 0x0}, {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
  {0x6, 0x5, 0x4, 0x0},
};

void isr() {
  uint8_t s = (uint8_t)((digitalRead(PIN_DT) << 1) | digitalRead(PIN_CLK));
  g_state = TT[g_state & 0x0f][s];
  if ((g_state & 0x30) == 0x10) ++g_pos;
  else if ((g_state & 0x30) == 0x20) --g_pos;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  pinMode(PIN_CLK, INPUT_PULLUP); pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);  pinMode(PIN_PWR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT),  isr, CHANGE);
  Serial.println(F("Encoder test: turn knob, press knob, press soft-power."));
}

void loop() {
  static int16_t last = 0;
  static bool sw = false, pwr = false;
  noInterrupts(); int16_t pos = g_pos; interrupts();
  if (pos != last) { Serial.print(F("pos=")); Serial.println(pos); last = pos; }

  bool swNow = (digitalRead(PIN_SW) == LOW);
  if (swNow && !sw) Serial.println(F("encoder SW pressed"));
  sw = swNow;
  bool pwrNow = (digitalRead(PIN_PWR) == LOW);
  if (pwrNow && !pwr) Serial.println(F("SOFT POWER pressed"));
  pwr = pwrNow;
  delay(5);
}
