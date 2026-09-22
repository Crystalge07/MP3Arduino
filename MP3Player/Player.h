// Playback logic and state. Talks to the DFPlayer (stage 3) or simulates it.
#pragma once
#include <Arduino.h>
#include "config.h"

enum PlayStatus : uint8_t {
  PS_STOPPED,   // nothing started yet: Play must start a track from the beginning
  PS_PLAYING,
  PS_PAUSED,    // Play resumes where it left off
};

enum PlayerError : uint8_t {
  PE_NONE,
  PE_NO_RESPONSE,   // no reply over serial: wiring/power
  PE_NO_FILES,      // replied, but no tracks: SD card/files
  PE_CARD_REMOVED,
};

// Everything the display needs. `version` changes whenever any other field
// does, so the display can redraw only when something actually changed.
struct PlayerState {
  uint16_t    track;        // 1-based
  uint16_t    trackCount;
  uint8_t     volume;
  PlayStatus  status;
  PlayerError error;
  bool        simulated;
  uint8_t     version;
};

class Player {
public:
  // Connects to the player and reads the track count.
  void begin();
  // Call every loop() pass: handles track-finished and other player events.
  void update(uint32_t now);

  void togglePlay();
  void next();       // next/previous always start playing, wrapping around
  void previous();
  void volumeUp();
  void volumeDown();

  const PlayerState &state() const { return _s; }

  static const __FlashStringHelper *errorTitle(PlayerError e);
  static const __FlashStringHelper *errorHint(PlayerError e, uint8_t line);

private:
  void playTrack(uint16_t track);
  void onTrackFinished();
  void setError(PlayerError e);
  void changed();

  PlayerState _s = { 1, 0, VOLUME_DEFAULT, PS_STOPPED, PE_NONE, !USE_DFPLAYER, 0 };

  // Simulation: how far into the fake track we are.
  uint32_t _simPlayedMs = 0;    // play time before the latest resume
  uint32_t _simResumedAt = 0;   // millis() at the latest play/resume
};
