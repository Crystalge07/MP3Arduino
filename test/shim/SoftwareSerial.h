#pragma once
#include <stdint.h>

class SoftwareSerial {
public:
  SoftwareSerial(uint8_t, uint8_t) {}
  void begin(long) {}
};
