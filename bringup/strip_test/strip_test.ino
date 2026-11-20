// Bring-up step 5: SK6812 RGBW strip.
//
// Cycles pure R, G, B and W at low brightness, lights the last pixel, then
// goes full white about 4 s in. Full white needs tail-end power injection;
// on head-end power only, unplug before then.
//   Data = D6 through 330 ohm, 60 pixels.
#include <Adafruit_NeoPixel.h>

constexpr uint8_t  PIN_DIN = 6;
constexpr uint16_t N = 60;
Adafruit_NeoPixel strip(N, PIN_DIN, NEO_GRBW + NEO_KHZ800);

// Shows one color on every pixel.
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
  fill(20, 0, 0, 0); delay(700);
  fill(0, 20, 0, 0); delay(700);
  fill(0, 0, 20, 0); delay(700);
  fill(0, 0, 0, 20); delay(700);

  // Only the last pixel, to check the count.
  strip.clear();
  strip.setPixelColor(N - 1, strip.Color(0, 0, 0, 60));
  strip.show();
  delay(1500);

  // Needs tail-end power injection.
  fill(255, 255, 255, 255);
  delay(2000);
}
