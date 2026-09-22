// Boot animation: a little cassette tape with a face hops in, spins its
// reels, blinks, sends up some music notes, then says hi.
#pragma once
#include <U8g2lib.h>

constexpr uint16_t BOOT_ANIM_MS  = 2800;  // total length
constexpr uint16_t BOOT_FRAME_MS = 50;    // at most ~20 fps; a page-mode redraw takes ~25 ms anyway

// Draws the frame for `t` ms into the animation. Must only draw (no state
// changes): in page-buffer mode U8g2 calls it 8 times per frame, once per
// 8-pixel strip, and every call has to draw the same picture.
void drawBootFrame(U8G2 &g, uint16_t t);
