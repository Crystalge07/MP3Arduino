#include "Player.h"
#include "debug.h"

#if USE_DFPLAYER
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

static SoftwareSerial dfSerial(PIN_DF_RX, PIN_DF_TX);
static DFRobotDFPlayerMini df;
#endif

void Player::begin() {
#if USE_DFPLAYER
  dfSerial.begin(9600);   // the DFPlayer only speaks 9600 baud
  // doReset = false: send nothing and don't wait. The DFPlayer boots by itself
  // at power-up; connect() talks to it later. With isACK = false, begin()
  // always returns true, so its result is ignored; connect() checks for a
  // real reply instead.
  df.begin(dfSerial, DF_USE_ACK, false);
#endif
}

void Player::connect() {
  _s.error = PE_NONE;
  _s.status = PS_STOPPED;
  _s.track = 1;
  _reconnectPending = false;

#if USE_DFPLAYER
  // Throw away anything the DFPlayer sent by itself, like the "card online"
  // message at power-up. Left in the buffer, it would be read as the reply
  // to the query below, and the query would fail.
  while (df.available()) df.readType();

  // Ask how many files are on the card. Each try waits up to 500 ms for a
  // reply; this only happens at boot, on retry, or after a card swap.
  int count = -1;
  bool heard = false;   // did the DFPlayer reply at all?
  for (uint8_t i = 0; i < 3; i++) {
    count = df.readFileCounts();
    if (count >= 0) break;
    // After a failed query the library keeps the last message type:
    // TimeOut = silence; DFPlayerError = it replied with an error (no card).
    uint8_t type = df.readType();
    if (type != TimeOut) heard = true;
    if (type == DFPlayerError) break;
  }
  if (count < 0) {
    setError(heard ? PE_NO_FILES : PE_NO_RESPONSE);
    return;
  }
  if (TRACK_COUNT_OVERRIDE) count = TRACK_COUNT_OVERRIDE;
  if (count == 0) {
    setError(PE_NO_FILES);
    return;
  }
  _s.trackCount = count;

  // Uploading a sketch resets the Uno but not the DFPlayer, which may still
  // be playing the old track; stop it so the state matches what we show.
  df.stop();
  df.volume(_s.volume);
  df.EQ(_s.eq);
  DBG(F("[player] DFPlayer ok, "));
#else
  _s.trackCount = SIM_TRACK_COUNT;
  if (SIM_FORCE_ERROR) {
    setError((PlayerError)SIM_FORCE_ERROR);
    return;
  }
  DBG(F("[player] simulated, "));
#endif
  DBG(_s.trackCount);
  DBGLN(F(" tracks"));
  changed();
}

void Player::update(uint32_t now) {
  if (_reconnectPending && now - _reconnectRequestedAt >= DF_CARD_MOUNT_MS) {
    connect();
    return;
  }

#if USE_DFPLAYER
  // available() only decodes bytes that have already arrived; it never waits.
  // Each event gets a fresh millis(), not `now`: a button earlier in this
  // pass, or an earlier event in this loop, may have started a track and
  // stamped _trackStartedAt with a time later than `now`. `now - later`
  // wraps to ~4 billion and would defeat the duplicate-"finished" guard.
  while (df.available()) {
    uint8_t type = df.readType();
    uint16_t value = df.read();
    handleEvent(type, value, millis());
  }
#else
  // A real DFPlayer reports "finished" over serial; here we fake it by time.
  // millis(), not `now`, for the same reason as above: a button press this
  // pass may have set _simResumedAt after `now` was read.
  if (_s.status == PS_PLAYING &&
      _simPlayedMs + (millis() - _simResumedAt) >= SIM_TRACK_MS) {
    DBGLN(F("[player] track finished"));
    onTrackFinished();
  }
#endif
}

#if USE_DFPLAYER
void Player::handleEvent(uint8_t type, uint16_t value, uint32_t now) {
  switch (type) {
    case DFPlayerPlayFinished:
      // Ignore "finished" when nothing should be playing (e.g. after stop()),
      // and right after a track starts: many modules send the event twice,
      // and some send it for the track a new play command interrupted.
      // Either would otherwise make auto-advance skip a track.
      if (_s.status != PS_PLAYING || now - _trackStartedAt < DF_FINISH_GUARD_MS) {
        DBGLN(F("[df] finished (ignored)"));
        break;
      }
      DBGLN(F("[df] track finished"));
      onTrackFinished();
      break;

    case DFPlayerCardRemoved:
      setError(PE_CARD_REMOVED);
      break;

    case DFPlayerCardInserted:
    case DFPlayerCardOnline:
      if (_s.error != PE_NONE) {
        DBGLN(F("[df] card inserted"));
        _reconnectPending = true;   // re-read the card once it has mounted
        _reconnectRequestedAt = now;
      }
      break;

    case DFPlayerError:
      // 1 busy, 2 sleeping, 3 bad frame, 4 bad checksum,
      // 5 track number out of range, 6 file not found.
      DBG(F("[df] error "));
      DBGLN(value);
      if ((value == FileIndexOut || value == FileMismatch) && _s.status == PS_PLAYING) {
        onTrackMissing();
      }
      break;

    default:
      DBG(F("[df] message type "));
      DBG(type);
      DBG(F(", value "));
      DBGLN(value);
      break;
  }
}

// The DFPlayer has no /mp3/NNNN.mp3 for the current track. The track count
// it reported covers every file on the card, hidden macOS "._" files and
// all, so it is often too high. Running into a missing number going forward,
// or right after wrapping back from track 1 to the last one, means the real
// playlist ends before it: shrink the count and carry on.
//
// This always ends: every path either stops, shows an error, shrinks the
// count, or steps backward toward track 1, whose absence is an error.
void Player::onTrackMissing() {
  uint16_t t = _s.track;
  if (t == 1) {                  // /mp3/0001.mp3 must exist
    setError(PE_NO_FILES);
    return;
  }
  if (_move == MOVE_PREV && t < _s.trackCount) {
    // A gap in the numbering: keep going the way the user was going.
    playTrack(t - 1, MOVE_PREV);
    return;
  }

  _s.trackCount = t - 1;
  Serial.print(F("WARN: no /mp3 file for track "));
  Serial.print(t);
  Serial.print(F(", playlist is now 1-"));
  Serial.print(_s.trackCount);
  Serial.println(F(" (extra files on the card?)"));

  if (_move == MOVE_PREV) {
    playTrack(t - 1, MOVE_PREV);   // land on the real last track
  } else if (_move == MOVE_AUTO && !LOOP_PLAYLIST) {
    _s.track = t - 1;              // ran off the end: stop, as onTrackFinished() does
    _s.status = PS_STOPPED;
    changed();
  } else {
    playTrack(1, _move);           // wrap, as next() does at the end
  }
}
#endif

void Player::togglePlay() {
  if (_s.error != PE_NONE) {
    connect();                 // on an error screen, Play means "try again"
    return;
  }
  switch (_s.status) {
    case PS_STOPPED:
      playTrack(_s.track, MOVE_PLAY);
      return;
    case PS_PLAYING:
#if USE_DFPLAYER
      df.pause();
#else
      _simPlayedMs += millis() - _simResumedAt;
#endif
      _s.status = PS_PAUSED;
      break;
    case PS_PAUSED:
#if USE_DFPLAYER
      df.start();
#else
      _simResumedAt = millis();
#endif
      _s.status = PS_PLAYING;
      break;
  }
  changed();
}

void Player::next() {
  if (_s.error != PE_NONE) return;
  playTrack(_s.track >= _s.trackCount ? 1 : _s.track + 1, MOVE_NEXT);
}

void Player::previous() {
  if (_s.error != PE_NONE) return;
  playTrack(_s.track <= 1 ? _s.trackCount : _s.track - 1, MOVE_PREV);
}

void Player::volumeUp() {
  if (_s.volume < VOLUME_MAX) setVolume(_s.volume + 1);
}

void Player::volumeDown() {
  if (_s.volume > 0) setVolume(_s.volume - 1);
}

void Player::setVolume(uint8_t v) {
  _s.volume = v;
#if USE_DFPLAYER
  df.volume(v);
#endif
  changed();
}

void Player::cycleEq() {
  _s.eq = (_s.eq + 1) % EQ_COUNT;
#if USE_DFPLAYER
  df.EQ(_s.eq);   // the numbering matches DFPLAYER_EQ_NORMAL..DFPLAYER_EQ_BASS
#endif
  DBG(F("[player] EQ "));
  DBGLN(eqName(_s.eq));
  changed();
}

void Player::playTrack(uint16_t track, Move how) {
  _s.track = track;
  _s.status = PS_PLAYING;
  _move = how;
  _trackStartedAt = millis();
#if USE_DFPLAYER
  // Plays /mp3/NNNN.mp3 chosen by file name. (play(n) and next() would go by
  // the order files were copied to the card instead.)
  df.playMp3Folder(track);
#else
  _simPlayedMs = 0;
  _simResumedAt = _trackStartedAt;
#endif
  changed();
}

void Player::onTrackFinished() {
  if (_s.track < _s.trackCount) {
    playTrack(_s.track + 1, MOVE_AUTO);
  } else if (LOOP_PLAYLIST) {
    playTrack(1, MOVE_AUTO);
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

// Shown in the OLED's top bar, so at most 7 characters.
const __FlashStringHelper *Player::eqName(uint8_t eq) {
  switch (eq) {
    case 1:  return F("POP");
    case 2:  return F("ROCK");
    case 3:  return F("JAZZ");
    case 4:  return F("CLASSIC");
    case 5:  return F("BASS");
    default: return F("NORMAL");
  }
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
