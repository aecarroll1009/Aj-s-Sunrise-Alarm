// Bring-up step 6: DFPlayer Mini and speaker.
//
// Loops /mp3/0001.mp3 and prints the BUSY pin each second. Pick a volume
// here, then copy it to ALARM_VOLUME in src/Tuning.h.
//   D7 --[1k]--> DFPlayer RX   D8 <-- DFPlayer TX   D9 <-- BUSY
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

constexpr uint8_t  PIN_DF_RX   = 7;    // to DFPlayer RX (Nano TX)
constexpr uint8_t  PIN_DF_TX   = 8;    // from DFPlayer TX (Nano RX)
constexpr uint8_t  PIN_BUSY    = 9;
constexpr uint8_t  TRACK       = 1;    // /mp3/0001.mp3
constexpr uint8_t  VOLUME      = 20;   // 0..30
constexpr uint32_t MIN_PLAY_MS = 800;  // BUSY lags a play command

SoftwareSerial ss(PIN_DF_TX, PIN_DF_RX);   // (rx, tx)
DFRobotDFPlayerMini df;
uint32_t startedMs = 0;
uint16_t plays = 0;
uint32_t lastReportMs = 0;

// Plays the track from the start and logs it.
void startTrack() {
  df.playMp3Folder(TRACK);
  startedMs = millis();
  ++plays;
  Serial.print(F("Playing /mp3/0001.mp3, start #"));
  Serial.println(plays);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  pinMode(PIN_BUSY, INPUT_PULLUP);
  ss.begin(9600);
  if (!df.begin(ss, /*isACK=*/false, /*doReset=*/true)) {
    Serial.println(F("DFPlayer begin() reported not ready (ok if SD present) - continuing"));
  }
  delay(1000);  // the module ignores commands right after reset
  df.volume(VOLUME);
  startTrack();
}

void loop() {
  const uint32_t now = millis();
  const bool busy = (digitalRead(PIN_BUSY) == LOW);

  if ((now - startedMs) > MIN_PLAY_MS && !busy) {
    startTrack();
  }
  if (now - lastReportMs >= 1000) {
    lastReportMs = now;
    Serial.print(F("BUSY pin: "));
    Serial.println(busy ? F("playing") : F("idle"));
  }
}
