// Settings kept in EEPROM across power-offs: volume, EQ, shuffle, last track.
#pragma once
#include <Arduino.h>

struct Saved {
  uint8_t  volume;
  uint8_t  eq;
  bool     shuffle;
  uint16_t track;
  uint16_t boots;   // power-on counter, mixed into the shuffle's random seed
};

// Returns false if nothing valid was saved yet (new board, or the layout
// changed); `s` is then left untouched.
bool settingsLoad(Saved &s);

// Only bytes that differ are written. EEPROM cells survive ~100,000 writes,
// so callers should batch changes rather than save on every button press.
void settingsSave(const Saved &s);
