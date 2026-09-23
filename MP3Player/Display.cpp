#include "Display.h"
#include "config.h"
#include "debug.h"

#if USE_DISPLAY
#include <Wire.h>
#include <U8g2lib.h>
#include "BootAnim.h"

// _1_ = page-buffer mode: a 128-byte buffer (one 8-pixel-tall strip) instead
// of the full 1 KB frame. Each redraw runs the drawing code 8 times, once per
// strip, which is why drawing must be done between firstPage()/nextPage().
#if OLED_IS_SSD1306
static U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
static U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#endif

// Fonts: _tr = ASCII only, _tn = digits only; much smaller in flash than _tf.
#define FONT_SMALL u8g2_font_5x8_tr       // 5 px wide, monospace
#define FONT_TITLE u8g2_font_6x10_tr      // 6 px wide, monospace
#define FONT_BIG   u8g2_font_logisoso20_tn

bool Display::begin() {
  Wire.begin();
  // If SDA/SCL are miswired or shorted, the stock Wire library can wait
  // forever for the bus. With a timeout (3 ms) it gives up and resets instead.
  Wire.setWireTimeout(3000, true);

  // U8g2 never checks for an I2C acknowledgement, so it "succeeds" even with
  // nothing connected. Ask the address ourselves first.
  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.print(F("ERROR: no OLED at I2C 0x"));
    Serial.print(OLED_I2C_ADDR, HEX);
    Serial.println(F(" (check SDA=A4, SCL=A5, VCC/GND order, address)"));
    return false;
  }

  u8g2.setI2CAddress(OLED_I2C_ADDR << 1);  // U8g2 wants the 8-bit form
  u8g2.setBusClock(OLED_I2C_CLOCK);
  u8g2.begin();
  _present = true;
  DBGLN(F("[oled] ready"));
  return true;
}

void Display::startBoot(uint32_t now) {
  if (!_present) return;
  _mode = MODE_BOOT;
  _bootStart = now;
  _lastFrame = now - BOOT_FRAME_MS;   // draw the first frame immediately
}

void Display::skipBoot() {
  if (_mode != MODE_BOOT) return;
  _mode = MODE_MAIN;
  _dirty = true;
}

// Writes n (1..65535) into buf and returns its length.
static uint8_t toStr(uint16_t n, char *buf) {
  utoa(n, buf, 10);
  return strlen(buf);
}

static void drawStatusIcon(PlayStatus st) {
  switch (st) {
    case PS_PLAYING: u8g2.drawTriangle(0, 1, 0, 9, 5, 5); break;       // >
    case PS_PAUSED:  u8g2.drawBox(0, 1, 2, 8); u8g2.drawBox(4, 1, 2, 8); break;  // ||
    default:         u8g2.drawBox(0, 2, 6, 6); break;                  // []
  }
}

static void drawMain(const PlayerState &s) {
  char num[6], count[7];

  // Top bar: status on the left, EQ preset right-aligned, SIM marker between.
  drawStatusIcon(s.status);
  u8g2.setFont(FONT_SMALL);
  u8g2.setCursor(10, 8);
  u8g2.print(s.status == PS_PLAYING ? F("PLAYING") :
             s.status == PS_PAUSED  ? F("PAUSED")  : F("READY"));
  if (s.simulated) {
    u8g2.setCursor(62, 8);
    u8g2.print(F("SIM"));
  }
  const __FlashStringHelper *eq = Player::eqName(s.eq);
  u8g2.setCursor(128 - 5 * strlen_P((PGM_P)eq), 8);   // F() strings live in flash
  u8g2.print(eq);
  u8g2.drawHLine(0, 11, 128);

  // Centre: "TRACK", then a big track number followed by a small "/count".
  u8g2.setCursor((128 - 5 * 5) / 2, 20);
  u8g2.print(F("TRACK"));

  toStr(s.track, num);
  count[0] = '/';
  uint8_t countW = (toStr(s.trackCount, count + 1) + 1) * 5;
  u8g2.setFont(FONT_BIG);
  uint8_t numW = u8g2.getStrWidth(num);   // logisoso is proportional: measure it
  uint8_t x = (128 - (numW + 3 + countW)) / 2;
  u8g2.drawStr(x, 43, num);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(x + numW + 3, 43, count);

  // Bottom: speaker icon, volume bar, number.
  u8g2.drawBox(0, 53, 3, 5);
  u8g2.drawTriangle(3, 55, 7, 51, 7, 60);
  u8g2.drawFrame(11, 51, 94, 9);
  uint8_t fill = (uint16_t)90 * s.volume / VOLUME_MAX;
  if (fill) u8g2.drawBox(13, 53, fill, 5);
  toStr(s.volume, num);
  u8g2.drawStr(109, 59, num);
}

static void drawError(const PlayerState &s) {
  // Warning triangle with "!".
  u8g2.drawLine(12, 1, 1, 22);
  u8g2.drawLine(12, 1, 23, 22);
  u8g2.drawHLine(1, 22, 23);
  u8g2.drawBox(11, 8, 2, 8);
  u8g2.drawBox(11, 18, 2, 2);

  u8g2.setFont(FONT_TITLE);
  u8g2.setCursor(30, 16);
  u8g2.print(Player::errorTitle(s.error));

  u8g2.setFont(FONT_SMALL);
  for (uint8_t i = 0; i < 2; i++) {
    u8g2.setCursor(0, 36 + 10 * i);
    u8g2.print(Player::errorHint(s.error, i));
  }
  u8g2.drawHLine(0, 52, 128);
  u8g2.setCursor((128 - 19 * 5) / 2, 62);
  u8g2.print(F("Press Play to retry"));
}

void Display::update(uint32_t now, const PlayerState &s) {
  if (!_present) return;

  if (_mode == MODE_BOOT) {
    uint32_t t = now - _bootStart;
    if (t < BOOT_ANIM_MS) {
      if (now - _lastFrame >= BOOT_FRAME_MS) {
        _lastFrame = now;
        u8g2.firstPage();
        do { drawBootFrame(u8g2, t); } while (u8g2.nextPage());
      }
      return;
    }
    _mode = MODE_MAIN;
    _dirty = true;
  }

  if (!_dirty && s.version == _drawnVersion) return;
  _dirty = false;
  _drawnVersion = s.version;

#if DEBUG
  uint32_t t0 = millis();
#endif
  u8g2.firstPage();
  do {
    if (s.error != PE_NONE) drawError(s);
    else drawMain(s);
  } while (u8g2.nextPage());
  DBG(F("[oled] redraw "));
  DBG(millis() - t0);
  DBGLN(F(" ms"));
}

#else  // USE_DISPLAY == 0: no OLED; everything is a no-op.

bool Display::begin() { return true; }
void Display::startBoot(uint32_t) {}
void Display::skipBoot() {}
void Display::update(uint32_t, const PlayerState &) {}

#endif
