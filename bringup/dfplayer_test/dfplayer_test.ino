// Bring-up step 6 (spec 3.8): DFPlayer Mini and speaker.
//
// Plays /mp3/0001.mp3 on a loop and reports the BUSY pin once a second. When
// BUSY goes idle the track is started again, the same way the firmware
// repeats the song until the alarm is snoozed or dismissed. This is
// write-only: it never reads the module's serial line, since that can collide
// with the strip later. Set a comfortable volume here; the real value goes in
// Config.h at the end.
//   Nano D7 --[1k]--> DFPlayer RX     Nano D8 <-- DFPlayer TX     BUSY --> D9
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

constexpr uint8_t  PIN_DF_RX   = 7;    // to DFPlayer RX (Nano TX)
constexpr uint8_t  PIN_DF_TX   = 8;    // from DFPlayer TX (Nano RX)
constexpr uint8_t  PIN_BUSY    = 9;
constexpr uint8_t  TRACK       = 1;    // /mp3/0001.mp3
constexpr uint8_t  VOLUME      = 20;   // 0..30
constexpr uint32_t MIN_PLAY_MS = 800;  // ignore BUSY for this long after starting the track

SoftwareSerial ss(PIN_DF_TX, PIN_DF_RX);   // (rx, tx)
DFRobotDFPlayerMini df;
uint32_t startedMs = 0;
uint16_t plays = 0;
uint32_t lastReportMs = 0;

// Starts /mp3/0001.mp3 from the beginning and counts the play.
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
  delay(1000);  // the module needs a moment after reset before it takes commands
  df.volume(VOLUME);
  startTrack();
}

void loop() {
  const uint32_t now = millis();
  const bool busy = (digitalRead(PIN_BUSY) == LOW);  // low means a track is playing

  if ((now - startedMs) > MIN_PLAY_MS && !busy) {
    startTrack();
  }
  if (now - lastReportMs >= 1000) {
    lastReportMs = now;
    Serial.print(F("BUSY pin: "));
    Serial.println(busy ? F("playing") : F("idle"));
  }
}
