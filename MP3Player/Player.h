// Playback logic and state. Talks to the DFPlayer, or simulates it when
// USE_DFPLAYER is 0.
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
  PE_NO_FILES,      // replied, but no card or no playable tracks
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
  // Call in setup(): opens the serial link. Sends nothing and doesn't wait.
  void begin();
  // Checks the DFPlayer answers and reads the track count. Call once it has
  // had DF_BOOT_MS to start up. Blocks up to ~1.5 s if it doesn't answer.
  void connect();
  // Call every loop() pass: handles track-finished and card events.
  void update(uint32_t now);

  void togglePlay(); // on an error screen, retries connect() instead
  void next();       // next/previous always start playing, wrapping around
  void previous();
  void volumeUp();
  void volumeDown();

  const PlayerState &state() const { return _s; }

  static const __FlashStringHelper *errorTitle(PlayerError e);
  static const __FlashStringHelper *errorHint(PlayerError e, uint8_t line);

private:
  void playTrack(uint16_t track);
  void setVolume(uint8_t v);
  void onTrackFinished();
  void handleEvent(uint8_t type, uint16_t value, uint32_t now);
  void setError(PlayerError e);
  void changed();

  PlayerState _s = { 1, 0, VOLUME_DEFAULT, PS_STOPPED, PE_NONE, !USE_DFPLAYER, 0 };

  uint32_t _trackStartedAt = 0;
  bool     _reconnectPending = false;   // card reinserted: connect() soon
  uint32_t _reconnectRequestedAt = 0;
  uint8_t  _playErrors = 0;             // "file not found" errors in a row

#if !USE_DFPLAYER
  uint32_t _simPlayedMs = 0;    // play time before the latest resume
  uint32_t _simResumedAt = 0;   // millis() at the latest play/resume
#endif
};
