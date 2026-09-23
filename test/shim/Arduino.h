// Just enough of the Arduino API to build Player.cpp on a PC.
// millis() is a clock the tests move by hand; Serial collects its output.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <type_traits>

class __FlashStringHelper;
#define F(s)     (reinterpret_cast<const __FlashStringHelper *>(s))
#define PGM_P    const char *
#define strlen_P strlen
#define DEC 10
#define HEX 16

extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }

struct FakeSerial {
  std::string out;

  void print(const char *s) { out += s; }
  void print(const __FlashStringHelper *s) { out += reinterpret_cast<const char *>(s); }
  void print(char c) { out += c; }
  template <class T>
  void print(T v, int base = DEC) {
    static_assert(std::is_integral<T>::value, "numbers only");
    char buf[24];
    if (base == HEX) snprintf(buf, sizeof buf, "%llX", (unsigned long long)v);
    else if (std::is_signed<T>::value) snprintf(buf, sizeof buf, "%lld", (long long)v);
    else snprintf(buf, sizeof buf, "%llu", (unsigned long long)v);
    out += buf;
  }

  template <class... A>
  void println(A... a) { print(a...); out += "\r\n"; }
  void println() { out += "\r\n"; }
};
extern FakeSerial Serial;
