#include "config.h"
#if USE_DISPLAY
#include "BootAnim.h"
#include <avr/pgmspace.h>

// Timeline (ms): hop in 0-460, reels spin from 460, blinks at 1000 and 1700,
// notes float up from 600, happy face + "hi!" from 2000.

constexpr int CX = 64;       // cassette centre x
constexpr int REST_Y = 22;   // cassette top edge once it has landed

// Reel spoke endpoints (dx, dy) at radius 4, for 4 rotation phases.
// Three spokes 120 deg apart look identical after a 120 deg turn, so 4 steps
// of 30 deg are a full visual cycle, with no sin()/cos() or float code
// (floating-point math would add ~1-2 KB of flash on the AVR).
static const int8_t SPOKES[4][3][2] PROGMEM = {
  { { 4, 0 }, { -2, 3 }, { -2, -3 } },   //   0, 120, 240 deg
  { { 3, 2 }, { -3, 2 }, {  0, -4 } },   //  30, 150, 270
  { { 2, 3 }, { -4, 0 }, {  2, -3 } },   //  60, 180, 300
  { { 0, 4 }, { -3, -2 }, { 3, -2 } },   //  90, 210, 330
};

static int cassetteY(uint16_t t) {
  if (t < 300) return 2 + (REST_Y - 2) * (int)t / 300;       // drop
  if (t < 380) return REST_Y - 3 * (int)(t - 300) / 80;      // bounce up...
  if (t < 460) return REST_Y - 3 + 3 * (int)(t - 380) / 80;  // ...and settle
  return REST_Y;
}

static void drawReel(U8G2 &g, int cx, int cy, uint8_t phase) {
  g.drawCircle(cx, cy, 5);
  for (uint8_t k = 0; k < 3; k++) {
    int8_t dx = (int8_t)pgm_read_byte(&SPOKES[phase][k][0]);
    int8_t dy = (int8_t)pgm_read_byte(&SPOKES[phase][k][1]);
    g.drawLine(cx, cy, cx + dx, cy + dy);
  }
}

static void drawEye(U8G2 &g, int x, int y, bool blink, bool happy) {
  if (happy) {                       // ^
    g.drawLine(x - 2, y + 1, x, y - 1);
    g.drawLine(x, y - 1, x + 2, y + 1);
  } else if (blink) {                // -
    g.drawHLine(x - 2, y, 5);
  } else {                           // o
    g.drawDisc(x, y, 2);
  }
}

static void drawCassette(U8G2 &g, int y, uint16_t t) {
  g.drawRFrame(CX - 30, y, 60, 38, 4);        // body
  g.drawRFrame(CX - 24, y + 4, 48, 16, 2);    // label = face
  g.drawRFrame(CX - 19, y + 23, 38, 12, 3);   // reel window
  g.drawDisc(CX - 24, y + 33, 1);             // screw holes
  g.drawDisc(CX + 23, y + 33, 1);

  bool blink = (t >= 1000 && t < 1120) || (t >= 1700 && t < 1820);
  bool happy = t >= 2000;
  drawEye(g, CX - 8, y + 10, blink, happy);
  drawEye(g, CX + 8, y + 10, blink, happy);

  g.drawPixel(CX - 3, y + 15);                // smile
  g.drawHLine(CX - 2, y + 16, 5);
  g.drawPixel(CX + 3, y + 15);
  g.drawHLine(CX - 15, y + 14, 3);            // blush
  g.drawHLine(CX + 13, y + 14, 3);

  uint8_t phase = t < 460 ? 0 : (t / 80) % 4;
  drawReel(g, CX - 10, y + 29, phase);
  drawReel(g, CX + 10, y + 29, phase);
}

static void drawNote(U8G2 &g, int x, int y) {   // (x, y) = centre of the note head
  g.drawDisc(x, y, 2);
  g.drawVLine(x + 2, y - 7, 7);
  g.drawLine(x + 2, y - 7, x + 5, y - 4);
}

// A note that floats up 20 px over 1 s starting at `start`, swaying 1 px.
static void drawFloatingNote(U8G2 &g, uint16_t t, uint16_t start, int x) {
  if (t < start || t - start >= 1000) return;
  uint16_t e = t - start;
  int y = 30 - (int)(e / 50);                    // 20 px per 1000 ms
  uint8_t p = (e / 100) % 4;
  int sway = (p == 1) - (p == 3);                // 0, +1, 0, -1
  if (y < 9) return;                             // keep the stem on screen
  drawNote(g, x + sway, y);
}

void drawBootFrame(U8G2 &g, uint16_t t) {
  drawCassette(g, cassetteY(t), t);
  drawFloatingNote(g, t, 600, 18);
  drawFloatingNote(g, t, 1100, 106);
  drawFloatingNote(g, t, 1500, 20);
  if (t >= 2000) {
    g.setFont(u8g2_font_6x10_tr);
    g.setCursor(CX - 9, 15);                     // "hi!" is 3 x 6 px wide
    g.print(F("hi!"));
  }
}

#endif  // USE_DISPLAY
