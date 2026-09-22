#include "Player.h"
#include "debug.h"

#if USE_DFPLAYER
  #error "Real DFPlayer support arrives in stage 3; set USE_DFPLAYER 0 for now."
#endif

// ---------------------------------------------------------------------------
// Player logic. Shared by the real and simulated player; only the commands
// that talk to the hardware differ.
// ---------------------------------------------------------------------------

void Player::begin() {
  _s.trackCount = SIM_TRACK_COUNT;
  DBG(F("[player] simulated, "));
  DBG(_s.trackCount);
  DBGLN(F(" tracks"));
  if (SIM_FORCE_ERROR) setError((PlayerError)SIM_FORCE_ERROR);
  changed();
}

void Player::update(uint32_t now) {
  if (_s.status != PS_PLAYING) return;
  // A real DFPlayer reports "finished" over serial; here we fake it by time.
  if (_simPlayedMs + (now - _simResumedAt) >= SIM_TRACK_MS) {
    DBGLN(F("[player] track finished"));
    onTrackFinished();
  }
}

void Player::togglePlay() {
  if (_s.error != PE_NONE) return;
  switch (_s.status) {
    case PS_STOPPED:
      playTrack(_s.track);
      return;
    case PS_PLAYING:
      _simPlayedMs += millis() - _simResumedAt;
      _s.status = PS_PAUSED;
      break;
    case PS_PAUSED:
      _simResumedAt = millis();
      _s.status = PS_PLAYING;
      break;
  }
  changed();
}

void Player::next() {
  if (_s.error != PE_NONE) return;
  playTrack(_s.track >= _s.trackCount ? 1 : _s.track + 1);
}

void Player::previous() {
  if (_s.error != PE_NONE) return;
  playTrack(_s.track <= 1 ? _s.trackCount : _s.track - 1);
}

void Player::volumeUp() {
  if (_s.volume >= VOLUME_MAX) return;
  _s.volume++;
  changed();
}

void Player::volumeDown() {
  if (_s.volume == 0) return;
  _s.volume--;
  changed();
}

void Player::playTrack(uint16_t track) {
  _s.track = track;
  _s.status = PS_PLAYING;
  _simPlayedMs = 0;
  _simResumedAt = millis();
  changed();
}

void Player::onTrackFinished() {
  if (_s.track < _s.trackCount || LOOP_PLAYLIST) {
    next();
  } else {
    _s.status = PS_STOPPED;   // end of playlist: next Play restarts this track
    changed();
  }
}

void Player::setError(PlayerError e) {
  _s.error = e;
  _s.status = PS_STOPPED;
  // Errors always go to Serial, even with DEBUG 0.
  Serial.print(F("ERROR: "));
  Serial.print(errorTitle(e));
  Serial.print(F(" - "));
  Serial.print(errorHint(e, 0));
  Serial.print(' ');
  Serial.println(errorHint(e, 1));
  changed();
}

void Player::changed() {
  _s.version++;
  DBG(F("[player] track "));
  DBG(_s.track);
  DBG('/');
  DBG(_s.trackCount);
  DBG(_s.status == PS_PLAYING ? F(" playing") :
      _s.status == PS_PAUSED  ? F(" paused")  : F(" stopped"));
  DBG(F(", vol "));
  DBGLN(_s.volume);
}

// Titles fit the OLED at 6 px/char (<= 16 chars); hints at 5 px (<= 25 chars).
const __FlashStringHelper *Player::errorTitle(PlayerError e) {
  switch (e) {
    case PE_NO_RESPONSE:  return F("DFPlayer silent");
    case PE_NO_FILES:     return F("No tracks found");
    case PE_CARD_REMOVED: return F("SD card removed");
    default:              return F("");
  }
}

const __FlashStringHelper *Player::errorHint(PlayerError e, uint8_t line) {
  switch (e) {
    case PE_NO_RESPONSE:
      return line == 0 ? F("Check TX/RX wiring,") : F("the 1k resistor and 5V.");
    case PE_NO_FILES:
      return line == 0 ? F("Insert FAT32 microSD") : F("with /mp3/0001.mp3 ...");
    case PE_CARD_REMOVED:
      return line == 0 ? F("Reinsert the card.") : F("");
    default:
      return F("");
  }
}
