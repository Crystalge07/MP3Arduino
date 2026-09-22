// Debug logging that compiles away entirely when DEBUG is 0.
//
//   DBG(F("volume "));  DBGLN(vol);  DBGLN(value, HEX);
//
// Always wrap literal text in F() so it's read from flash instead of being
// copied into the Uno's 2KB of RAM at startup.
#pragma once
#include "config.h"

#if DEBUG
  #define DBG(...)   Serial.print(__VA_ARGS__)
  #define DBGLN(...) Serial.println(__VA_ARGS__)
#else
  // do {} while (0) keeps the macro a single statement, so it stays safe
  // inside an if/else without braces.
  #define DBG(...)   do {} while (0)
  #define DBGLN(...) do {} while (0)
#endif
