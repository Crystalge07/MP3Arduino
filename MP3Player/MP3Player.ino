// MP3Player: Arduino Uno + DFPlayer Mini + SH1106 OLED + 5 buttons.
//
// This file only connects the modules. Pins and settings: config.h.
// Serial monitor: 115200 baud.

#include "config.h"
#include "debug.h"
#include "Buttons.h"
#include "Player.h"
#include "Display.h"

Buttons buttons;
Player  player;
Display display;

static bool booting = true;

static void handleButton(ButtonEvent ev) {
  DBG(F("[btn] "));
  DBGLN(Buttons::name(ev));
  switch (ev) {
    case BTN_PLAY:      player.togglePlay(); break;
    case BTN_NEXT:      player.next();       break;
    case BTN_PREV:      player.previous();   break;
    case BTN_VOL_UP:    player.volumeUp();   break;
    case BTN_VOL_DOWN:  player.volumeDown(); break;
    case BTN_PLAY_LONG: DBGLN(F("EQ: coming later")); break;
    default: break;
  }
}

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

  player.begin();    // opens the serial link; the DFPlayer boots meanwhile
  display.begin();   // logs its own error; the player works without it
  display.startBoot(millis());
}

void loop() {
  uint32_t now = millis();
  buttons.update(now);

  ButtonEvent ev;
  if (booting) {
    // Any press during the animation just skips it.
    while ((ev = buttons.poll()) != BTN_NONE) display.skipBoot();
    // The animation doubles as the DFPlayer's start-up time. If it was
    // skipped, or there's no display, wait out DF_BOOT_MS (without blocking).
    if (display.bootDone() && now >= DF_BOOT_MS) {
      player.connect();
      booting = false;
    }
  } else {
    while ((ev = buttons.poll()) != BTN_NONE) handleButton(ev);
    player.update(now);
  }

  display.update(now, player.state());
}
