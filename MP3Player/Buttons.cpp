#include "Buttons.h"
#include "config.h"
#include "debug.h"

// Same order as ButtonEvent (minus BTN_NONE).
static const uint8_t PINS[Buttons::COUNT] = {
  PIN_BTN_PLAY, PIN_BTN_NEXT, PIN_BTN_PREV, PIN_BTN_VOL_UP, PIN_BTN_VOL_DOWN,
};

static inline bool hasLongPress(uint8_t i) {
  return PLAY_LONG_PRESS && i == 0;
}

uint8_t Buttons::begin() {
  for (uint8_t i = 0; i < COUNT; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
  }
  delayMicroseconds(100);  // let the ~35k internal pull-ups charge the wiring (setup only)

  uint8_t heldMask = 0;
  uint32_t now = millis();
  for (uint8_t i = 0; i < COUNT; i++) {
    bool raw = digitalRead(PINS[i]) == LOW;
    // Start from the real state so a button held (or miswired) at boot
    // doesn't produce a phantom press.
    _btn[i] = { raw, raw, true, now, now };
    if (raw) heldMask |= 1 << i;
  }
  return heldMask;
}

void Buttons::update(uint32_t now) {
  for (uint8_t i = 0; i < COUNT; i++) {
    Button &b = _btn[i];
    bool raw = digitalRead(PINS[i]) == LOW;

    // Any raw change, including each bounce, restarts the settle timer.
    // Subtracting timestamps (now - then) stays correct when millis()
    // wraps around after ~49 days; comparing them directly would not.
    if (raw != b.rawPressed) {
      b.rawPressed = raw;
      b.rawChangedAt = now;
      continue;
    }

    // Raw has been steady for the debounce window: accept it.
    if (raw != b.pressed && now - b.rawChangedAt >= BUTTON_DEBOUNCE_MS) {
      b.pressed = raw;
      ButtonEvent ev = (ButtonEvent)(i + 1);
      if (raw) {
        b.pressedAt = now;
        b.longFired = false;
        if (!hasLongPress(i)) push(ev);          // act on press: feels instant
      } else if (hasLongPress(i) && !b.longFired) {
        push(ev);                                 // released before long: short press
      }
    }

    // Fire the long press while still held, so it happens without letting go.
    if (hasLongPress(i) && b.pressed && !b.longFired &&
        now - b.pressedAt >= LONG_PRESS_MS) {
      b.longFired = true;
      push(BTN_PLAY_LONG);
    }
  }
}

void Buttons::push(ButtonEvent e) {
  if (_count == QUEUE_SIZE) {
    DBGLN(F("[btn] queue full, dropped event"));
    return;
  }
  _queue[(_head + _count) % QUEUE_SIZE] = e;
  _count++;
}

ButtonEvent Buttons::poll() {
  if (_count == 0) return BTN_NONE;
  ButtonEvent e = _queue[_head];
  _head = (_head + 1) % QUEUE_SIZE;
  _count--;
  return e;
}

const __FlashStringHelper *Buttons::name(ButtonEvent e) {
  switch (e) {
    case BTN_PLAY:      return F("PLAY/PAUSE");
    case BTN_NEXT:      return F("NEXT");
    case BTN_PREV:      return F("PREV");
    case BTN_VOL_UP:    return F("VOL+");
    case BTN_VOL_DOWN:  return F("VOL-");
    case BTN_PLAY_LONG: return F("PLAY (long)");
    default:            return F("NONE");
  }
}
