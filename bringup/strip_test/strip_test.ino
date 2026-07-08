// Bring-up step 5 (spec SS 3.8): SK6812 RGBW strip.
// SEQUENCE THIS SAFELY:
//   1. HEAD power + data + 1000uF cap only. Run with LOW brightness first and
//      confirm the pixel COUNT (last lit pixel) and channel ORDER (R/G/B/W each
//      show pure). If white looks tinted, the color order isn't NEO_GRBW.
//   2. Add TAIL power injection, THEN run the full-white test and confirm the
//      far end holds ~5V (no dim/warm tail). Watch that the Nano doesn't reset.
//   Data = D6 via 330 ohm.  120 px.
#include <Adafruit_NeoPixel.h>

constexpr uint8_t  PIN_DIN = 6;
constexpr uint16_t N = 120;
Adafruit_NeoPixel strip(N, PIN_DIN, NEO_GRBW + NEO_KHZ800);

void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  uint32_t c = strip.Color(r, g, b, w);
  for (uint16_t i = 0; i < N; ++i) strip.setPixelColor(i, c);
  strip.show();
}

void setup() {
  strip.begin();
  strip.clear();
  strip.show();
}

void loop() {
  // Low-brightness channel walk (safe on HEAD-only power): R, G, B, W in turn.
  fill(20, 0, 0, 0); delay(700);   // pure red
  fill(0, 20, 0, 0); delay(700);   // pure green
  fill(0, 0, 20, 0); delay(700);   // pure blue
  fill(0, 0, 0, 20); delay(700);   // white channel only

  // Count check: light only the last pixel bright-ish.
  strip.clear();
  strip.setPixelColor(N - 1, strip.Color(0, 0, 0, 60));
  strip.show();
  delay(1500);

  // FULL WHITE — only after TAIL injection is in place.
  fill(255, 255, 255, 255);
  delay(2000);
}
