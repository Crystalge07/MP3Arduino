// Shuffle order for tracks 1..n: every track plays once before any repeats.
//
// Storing a shuffled list would take 2 bytes per track, too much of the Uno's
// 2 KB RAM for a big card. Instead, position -> track is computed by a small
// keyed scrambling function (a Feistel network) that is guaranteed to be a
// one-to-one mapping, so it's a true shuffle in 14 bytes.
#pragma once
#include <Arduino.h>

class Shuffle {
public:
  // New random order of 1..n whose first track is `first`.
  void begin(uint16_t n, uint16_t first);

  uint16_t track() const { return at(_pos); }
  bool atLast() const { return _pos + 1 >= _n; }

  // After the last track, a fresh order is drawn (not starting with the
  // track that just played).
  void next();
  void prev();       // wraps to the end of the current order

private:
  uint16_t scramble(uint16_t x) const;
  uint16_t at(uint16_t pos) const;

  uint16_t _n = 1;
  uint16_t _pos = 0;
  uint16_t _offset = 0;
  uint16_t _key[4] = {};
  uint8_t  _half = 1;   // bits per half of the scrambled value
};
