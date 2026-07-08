// Bring-up step 6 (spec SS 3.8): DFPlayer Mini + speaker.
// Plays /mp3/0001.mp3 and reports the BUSY pin. Write-only: we never read the
// module's serial (that can collide with the strip later). Set a comfortable
// volume here; the real value goes in Config.h at the end.
//   Nano D7 --[1k]--> DFPlayer RX     Nano D8 <-- DFPlayer TX     BUSY --> D9
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

constexpr uint8_t PIN_DF_RX = 7;   // to DFPlayer RX (Nano TX)
constexpr uint8_t PIN_DF_TX = 8;   // from DFPlayer TX (Nano RX)
constexpr uint8_t PIN_BUSY  = 9;

SoftwareSerial ss(PIN_DF_TX, PIN_DF_RX);   // (rx, tx)
DFRobotDFPlayerMini df;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  pinMode(PIN_BUSY, INPUT_PULLUP);
  ss.begin(9600);
  if (!df.begin(ss, /*isACK=*/false, /*doReset=*/true)) {
    Serial.println(F("DFPlayer begin() reported not ready (ok if SD present) - continuing"));
  }
  df.volume(20);         // 0..30
  df.play(1);            // /mp3/0001.mp3
  Serial.println(F("Playing track 1..."));
}

void loop() {
  Serial.print(F("BUSY pin: "));
  Serial.println(digitalRead(PIN_BUSY) == LOW ? F("playing") : F("idle"));
  delay(1000);
}
