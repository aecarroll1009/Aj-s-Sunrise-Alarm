#pragma once
#include <stdint.h>
#include "Types.h"

// Values to adjust on the finished lamp (spec 4.12). TODO(hardware) entries
// are first guesses.

// Lamp brightness (spec 4.2).
constexpr uint8_t DEFAULT_BRIGHTNESS = 200;
constexpr uint8_t MIN_BRIGHTNESS     = 8;  // lowest knob setting; the lamp stays lit

// Snooze length (spec 4.8).
constexpr uint32_t SNOOZE_MS = 10UL * 60UL * 1000UL;

// TODO(hardware): sunrise ramp (spec 4.7). Colors interpolate between
// keyframes, then get gamma shaping. progress is permille of the ramp.
constexpr uint32_t SUNRISE_DURATION_MS = 25UL * 60UL * 1000UL;  // spec: 20-30 min
constexpr uint8_t  SUNRISE_GAMMA       = 2;

struct SunriseKeyframe { uint16_t progress; Rgbw color; };

constexpr SunriseKeyframe SUNRISE_KEYFRAMES[] = {
  //  progress   R    G    B    W
  {     0,  {   1,   0,   0,   0 } },  // dim deep red, never fully off
  {   200,  {  60,   3,   0,   0 } },
  {   500,  { 200,  70,   0,   0 } },  // orange to amber
  {   850,  { 255, 150,  20,  90 } },  // warm white
  {  1000,  { 255, 180,  60, 255 } },  // bright white
};
constexpr uint8_t SUNRISE_KEYFRAME_COUNT =
    sizeof(SUNRISE_KEYFRAMES) / sizeof(SUNRISE_KEYFRAMES[0]);

// TODO(hardware): lamp color presets (spec 4.4, 4.6). Names fit in 15
// characters. Brightness is applied separately.
struct ColorPreset { const char* name; Rgbw color; };

constexpr ColorPreset COLOR_PRESETS[] = {
  { "Warm White",   { 255, 170,  70, 255 } },
  { "Bright White", { 255, 220, 180, 255 } },
  { "Deep Red",     { 255,   0,   0,   0 } },
  { "Amber",        { 255, 120,   0,   0 } },
  { "Ocean Blue",   {   0,  80, 255,   0 } },
  { "Forest",       {  20, 200,  40,   0 } },
};
constexpr uint8_t COLOR_PRESET_COUNT =
    sizeof(COLOR_PRESETS) / sizeof(COLOR_PRESETS[0]);

// TODO(hardware): wake song (spec 3.6), /mp3/0001.mp3 on the microSD.
constexpr uint8_t ALARM_TRACK  = 1;
constexpr uint8_t ALARM_VOLUME = 20;  // 0..30
