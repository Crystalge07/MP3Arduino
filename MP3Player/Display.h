// The OLED: boot animation, player screen, error screen.
// With USE_DISPLAY 0 every method does nothing, so the rest of the code
// doesn't need #ifs.
#pragma once
#include <Arduino.h>
#include "Player.h"

class Display {
public:
  // Checks the OLED answers on I2C, then initialises it. Returns false if
  // nothing answered; the display then stays off and everything else runs.
  bool begin();

  void startBoot(uint32_t now);
  void skipBoot();
  bool bootDone() const { return _mode != MODE_BOOT; }

  // Call every loop() pass. Redraws only when an animation frame is due or
  // the player state changed; a redraw blocks for ~25 ms.
  void update(uint32_t now, const PlayerState &s);

private:
  enum Mode : uint8_t { MODE_BOOT, MODE_MAIN };

  bool     _present = false;
  Mode     _mode = MODE_MAIN;
  bool     _dirty = true;         // force a redraw even if version matches
  uint8_t  _drawnVersion = 0;
  uint32_t _bootStart = 0;
  uint32_t _lastFrame = 0;
};
