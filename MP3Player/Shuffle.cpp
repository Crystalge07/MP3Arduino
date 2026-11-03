#include "Shuffle.h"

void Shuffle::begin(uint16_t n, uint16_t first) {
  _n = n ? n : 1;
  _pos = 0;
  // Scramble over the smallest even bit width that covers n.
  _half = 1;
  while ((1UL << (2 * _half)) < _n) _half++;
  for (uint8_t i = 0; i < 4; i++) _key[i] = random(0x10000);
  _offset = 0;
  // Rotate the order so that position 0 lands on `first`.
  uint16_t p0 = at(0) - 1;
  _offset = ((uint32_t)first - 1 + _n - p0) % _n;
}

// 4-round Feistel network on 2*_half bits. Whatever the round function does,
// this structure is always reversible, so distinct inputs give distinct outputs.
uint16_t Shuffle::scramble(uint16_t x) const {
  uint16_t mask = (1U << _half) - 1;
  uint16_t l = x >> _half, r = x & mask;
  for (uint8_t i = 0; i < 4; i++) {
    uint16_t f = (uint16_t)((r + _key[i]) * 0x9E5u);
    f ^= f >> 7;
    uint16_t t = l ^ (f & mask);
    l = r;
    r = t;
  }
  return (l << _half) | r;
}

uint16_t Shuffle::at(uint16_t pos) const {
  // The scramble covers a power of 4 >= n. "Cycle walking": re-scramble
  // until the result is in range; this keeps it one-to-one on 0..n-1 and
  // takes ~2 rounds on average.
  uint16_t x = pos;
  do { x = scramble(x); } while (x >= _n);
  return ((uint32_t)x + _offset) % _n + 1;
}

void Shuffle::next() {
  if (++_pos < _n) return;
  uint16_t last = at(_n - 1);
  uint16_t first = random(1, (long)_n + 1);
  if (first == last && _n > 1) first = first % _n + 1;
  begin(_n, first);
}

void Shuffle::prev() {
  _pos = _pos ? _pos - 1 : _n - 1;
}
