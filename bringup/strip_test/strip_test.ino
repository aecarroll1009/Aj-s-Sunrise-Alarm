// Bring-up step 5 (spec 3.8): SK6812 RGBW strip.
//
// Sequence this safely:
//   1. Power the strip from its head end only, with data and a 1000 uF cap.
//      Run at low brightness first and confirm the pixel count (the last
//      lit pixel) and the channel order (R/G/B/W each show pure). If white
//      looks tinted, the color order isn't NEO_GRBW.
//   2. Add power injection at the tail end, then run the full-white test
//      and confirm the far end still holds about 5V, with no dim or warm
//      tail. Watch that the Nano doesn't reset.
//   Data = D6 via 330 ohm. 120 px.
#include <Adafruit_NeoPixel.h>

constexpr uint8_t  PIN_DIN = 6;
constexpr uint16_t N = 120;
Adafruit_NeoPixel strip(N, PIN_DIN, NEO_GRBW + NEO_KHZ800);

// Fills every pixel with one color and shows it immediately.
//
// Args:
//   r: red channel, 0..255.
//   g: green channel, 0..255.
//   b: blue channel, 0..255.
//   w: white channel, 0..255.
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
  // Low-brightness channel walk (safe on head-only power): R, G, B, W in turn.
  fill(20, 0, 0, 0); delay(700);   // pure red
  fill(0, 20, 0, 0); delay(700);   // pure green
  fill(0, 0, 20, 0); delay(700);   // pure blue
  fill(0, 0, 0, 20); delay(700);   // white channel only

  // Count check: light only the last pixel, bright-ish.
  strip.clear();
  strip.setPixelColor(N - 1, strip.Color(0, 0, 0, 60));
  strip.show();
  delay(1500);

  // Full white; only run this once tail-end power injection is in place.
  fill(255, 255, 255, 255);
  delay(2000);
}
