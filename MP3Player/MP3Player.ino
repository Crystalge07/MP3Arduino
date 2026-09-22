// MP3Player: Arduino Uno + DFPlayer Mini + SH1106 OLED + 5 buttons.
//
// This file only connects the modules. Pins and settings: config.h.
// Stage 1: buttons only, events logged to Serial (115200 baud).

#include "config.h"
#include "debug.h"
#include "Buttons.h"

Buttons buttons;

void setup() {
  // Serial is always on so errors are visible even with DEBUG 0.
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("\nMP3Player booting"));

  uint8_t held = buttons.begin();
  for (uint8_t i = 0; i < Buttons::COUNT; i++) {
    if (held & (1 << i)) {
      Serial.print(F("WARN: button reads pressed at boot: "));
      Serial.print(Buttons::name((ButtonEvent)(i + 1)));
      Serial.println(F(" (check which switch legs are wired)"));
    }
  }
  DBGLN(F("Buttons ready"));
}

void loop() {
  uint32_t now = millis();
  buttons.update(now);

  ButtonEvent ev;
  while ((ev = buttons.poll()) != BTN_NONE) {
    DBG(F("[btn] "));
    DBG(Buttons::name(ev));
    DBG(F(" @ "));
    DBGLN(now);
  }
}
