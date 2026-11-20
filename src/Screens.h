#pragma once
#include <U8g2lib.h>
#include "UIMenu.h"

// OLED screen layouts (spec 4.4). Positions and fonts are a first cut.

// Lamp and alarm state shown on screen.
struct DisplayStatus {
  bool     lampOn;
  uint8_t  lampPreset;
  bool     alarmActive;
  bool     snoozing;
  uint32_t snoozeRemainingMs;
};

namespace Screens {

// Draws the alarm screen if the alarm is active, else the open menu screen.
// Call inside a U8g2 page loop.
//
// Args:
//   nowMs: current time, for blinking the field being edited.
void draw(U8G2& g, const UIMenu& ui, const DisplayStatus& s, uint32_t nowMs);

}  // namespace Screens
