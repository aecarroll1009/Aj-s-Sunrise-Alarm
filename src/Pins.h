#pragma once
#include <stdint.h>

// Wiring and part choices (spec 3.7). Included only by drivers.

// Nano pins. The OLED and DS3231 share I2C on A4 (SDA) and A5 (SCL).
constexpr uint8_t PIN_ENC_CLK   = 2;  // interrupt pin
constexpr uint8_t PIN_ENC_DT    = 3;  // interrupt pin
constexpr uint8_t PIN_ENC_SW    = 4;  // active-low
constexpr uint8_t PIN_SOFT_PWR  = 5;  // active-low
constexpr uint8_t PIN_STRIP_DIN = 6;  // through 330 ohm
constexpr uint8_t PIN_DF_RX     = 7;  // to DFPlayer RX, through 1 kohm
constexpr uint8_t PIN_DF_TX     = 8;  // from DFPlayer TX
constexpr uint8_t PIN_DF_BUSY   = 9;  // low while playing

// SK6812 RGBW strip, 1 m at 60 LED/m.
constexpr uint16_t LED_COUNT = 60;

// OLED controller. The panel is a 2.42" SSD1309.
#define OLED_SSD1306 0
#define OLED_SH1106  1
#define OLED_SSD1309 2
#define OLED_DRIVER  OLED_SSD1309
