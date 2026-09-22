// Debounced buttons that produce exactly one event per physical press.
#pragma once
#include <Arduino.h>

// Order matches the pin table in Buttons.cpp: button i produces event i + 1.
enum ButtonEvent : uint8_t {
  BTN_NONE = 0,
  BTN_PLAY,
  BTN_NEXT,
  BTN_PREV,
  BTN_VOL_UP,
  BTN_VOL_DOWN,
  BTN_PLAY_LONG,   // Play held past LONG_PRESS_MS (replaces BTN_PLAY)
};

class Buttons {
public:
  // Sets up the pins. Returns a bitmask of buttons that already read as
  // pressed (bit 0 = Play ... bit 4 = Vol-). Non-zero at boot usually means a
  // wiring fault, such as a 4-leg switch wired across its internally joined legs.
  uint8_t begin();

  // Call every loop() pass. Samples all buttons; never blocks.
  void update(uint32_t now);

  // Next pending event, or BTN_NONE.
  ButtonEvent poll();

  static const __FlashStringHelper *name(ButtonEvent e);

  static constexpr uint8_t COUNT = 5;

private:
  struct Button {
    bool     rawPressed;     // last raw reading, bounces and all
    bool     pressed;        // debounced state
    bool     longFired;      // long-press event already sent for this hold
    uint32_t rawChangedAt;   // when the raw reading last changed
    uint32_t pressedAt;      // when the debounced press began
  };

  void push(ButtonEvent e);

  Button _btn[COUNT];

  // Tiny FIFO so two buttons settling in the same pass both get delivered.
  static constexpr uint8_t QUEUE_SIZE = 4;
  ButtonEvent _queue[QUEUE_SIZE];
  uint8_t _head = 0;
  uint8_t _count = 0;
};
