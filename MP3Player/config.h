// All hardware pins and tunable settings live here.
//
// Compile-time switches are #defines because they're tested with #if.
// Everything else is constexpr: typed, checked by the compiler, and like a
// #define it occupies no RAM.
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Build stages: turn modules on as the hardware arrives.
// ---------------------------------------------------------------------------
#define USE_DISPLAY   1   // stage 2: SH1106 OLED
#ifndef USE_DFPLAYER      // the PC tests (test/Makefile) build both ways with -D
#define USE_DFPLAYER  1   // stage 3: DFPlayer Mini (0 = simulated player)
#endif

// 1 = verbose debug logging on Serial. Errors are always printed.
#define DEBUG         1
constexpr uint32_t SERIAL_BAUD = 115200;

// ---------------------------------------------------------------------------
// Buttons: other leg to GND, INPUT_PULLUP, so pressed reads LOW.
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_BTN_PLAY     = 2;
constexpr uint8_t PIN_BTN_NEXT     = 3;
constexpr uint8_t PIN_BTN_PREV     = 4;
constexpr uint8_t PIN_BTN_VOL_UP   = 5;
constexpr uint8_t PIN_BTN_VOL_DOWN = 6;

// How long a pin must hold steady before a change counts. Raise it if a worn
// button still double-triggers; 50 still feels instant.
constexpr uint16_t BUTTON_DEBOUNCE_MS = 25;

// Holding Play this long sends BTN_PLAY_LONG (cycles EQ) instead of
// play/pause. With this on, Play's short press fires on release rather than
// on press, since it can't know it was short until you let go.
#define PLAY_LONG_PRESS 1
constexpr uint16_t LONG_PRESS_MS = 700;

// ---------------------------------------------------------------------------
// DFPlayer Mini (used from stage 3)
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_DF_RX = 10;   // Uno RX <- DFPlayer TX
constexpr uint8_t PIN_DF_TX = 11;   // Uno TX -> 1k resistor -> DFPlayer RX

// Many clones never answer ACK requests. With ACK on they make every command
// wait for a reply that never comes. Try 1 only if you have a genuine module.
#define DF_USE_ACK 0

// The DFPlayer needs time after power-up to mount the SD card before it
// answers queries (longer with big cards/many files). The boot animation
// (2.8 s) normally covers this; the wait only matters with USE_DISPLAY 0.
constexpr uint32_t DF_BOOT_MS = 2000;
// "Track finished" events arriving this soon after starting a track are
// duplicates or refer to the interrupted track, and are ignored.
constexpr uint32_t DF_FINISH_GUARD_MS = 1500;
// After a card is reinserted, wait this long before reading it again.
constexpr uint32_t DF_CARD_MOUNT_MS = 1500;
// 0 = ask the DFPlayer how many tracks there are. Some clones report a wrong
// count; if yours does, put the real number of files here.
constexpr uint16_t TRACK_COUNT_OVERRIDE = 0;

constexpr uint8_t VOLUME_MAX     = 30;
constexpr uint8_t VOLUME_DEFAULT = 20;  // higher can brown out a USB-powered Uno

// DFPlayer EQ preset at boot: 0 Normal, 1 Pop, 2 Rock, 3 Jazz, 4 Classic,
// 5 Bass. Long-press Play cycles through them. (Some clones ignore EQ.)
constexpr uint8_t EQ_DEFAULT = 0;

// After the last track, go back to track 1 (true) or stop (false).
constexpr bool LOOP_PLAYLIST = true;

// Simulated player (USE_DFPLAYER 0): fake tracks so the UI can be tested.
constexpr uint16_t SIM_TRACK_COUNT = 12;
constexpr uint32_t SIM_TRACK_MS    = 8000;  // each fake "song" lasts 8 s
// Show an error screen for testing: 0 none, 1 no response, 2 no files,
// 3 card removed.
#define SIM_FORCE_ERROR 0

// ---------------------------------------------------------------------------
// OLED (used from stage 2)
// ---------------------------------------------------------------------------
constexpr uint8_t  OLED_I2C_ADDR  = 0x3C;    // 7-bit address; some boards use 0x3D
constexpr uint32_t OLED_I2C_CLOCK = 400000;  // 400 kHz: ~4x faster redraws than default
// 1.3" boards are usually SH1106. If the image is shifted 2 px with a stripe
// of noise at one edge, yours is really an SSD1306: set this to 1.
#define OLED_IS_SSD1306 0
