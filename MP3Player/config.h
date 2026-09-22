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
#define USE_DISPLAY   0   // stage 2: SH1106 OLED
#define USE_DFPLAYER  0   // stage 3: DFPlayer Mini (0 = simulated player)

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

// Holding Play this long sends BTN_PLAY_LONG (EQ cycling, later) instead of
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

constexpr uint8_t VOLUME_MAX     = 30;
constexpr uint8_t VOLUME_DEFAULT = 20;  // higher can brown out a USB-powered Uno

// ---------------------------------------------------------------------------
// OLED (used from stage 2)
// ---------------------------------------------------------------------------
constexpr uint8_t OLED_I2C_ADDR = 0x3C;  // 7-bit address; some boards use 0x3D
