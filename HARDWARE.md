# MP3Player hardware

Arduino Uno (or Nano) + DFPlayer Mini + 1.3" SH1106 OLED + 5 buttons + speaker.
Pin numbers live in `MP3Player/config.h`; this file must match it.

## Wiring

| From | To | Notes |
|---|---|---|
| Play / Next / Prev / Vol+ / Vol- buttons | D2 / D3 / D4 / D5 / D6 | Other leg to GND. No resistors (internal pull-ups). On 4-leg switches use diagonal legs. |
| DFPlayer TX | D10 | |
| D11 | 1 kΩ resistor → DFPlayer RX | Resistor stops a hum/hiss from the 5 V signal. |
| DFPlayer VCC / GND | 5V / GND | Put a 470–1000 µF capacitor across VCC/GND right at the DFPlayer. |
| DFPlayer SPK_1 / SPK_2 | Speaker | 4–8 Ω, up to 3 W. Neither speaker wire goes to GND. |
| OLED SDA / SCL | A4 / A5 | |
| OLED VCC / GND | 5V / GND | Check the order printed on your board; some swap VCC and GND. |
| A0 | nothing | Left floating on purpose: its noise seeds the shuffle. |

SD card: FAT32, songs in `/mp3/` named `0001.mp3`, `0002.mp3`, ... with no
gaps in the numbering. After copying from a Mac, run
`dot_clean /Volumes/<card>` before ejecting.

## Controls

| Button | Tap | Hold (0.7 s) |
|---|---|---|
| Play | Play / pause (retry on an error screen) | Next EQ preset |
| Next | Next track | Shuffle on/off |
| Prev | Previous track | |
| Vol+ / Vol- | Volume | |

Volume, EQ, shuffle and the current track are remembered after power-off.

## Battery power

Budget: Uno ~50 mA + OLED ~20 mA + DFPlayer ~20 mA idle, plus the speaker
(loud peaks reach several hundred mA). Plan for 150 mA average, 1 A peaks.

Parts:
- 1-cell LiPo, 3.7 V, 1000–2000 mAh, **with a protection circuit**
- TP4056 USB-C charger board (the version with protection, 6 pins/pads)
- MT3608 (or similar) boost converter, 3.7 V → 5 V, rated ≥ 2 A
- Slide switch

```
LiPo ─ TP4056 (B+/B-) ─ OUT+ ─ switch ─ MT3608 IN+ ── MT3608 OUT+ ─ Uno 5V pin
                         OUT- ─────────── MT3608 IN- ── MT3608 OUT- ─ Uno GND
```

- **Set the boost to 5.0 V with a multimeter BEFORE connecting it.** The 5V
  pin skips the Uno's regulator, so anything much over 5.5 V kills the board.
  (VIN needs 7 V+ and won't work from a 5 V boost.)
- **Switch the battery off before plugging in USB** to upload code; otherwise
  the boost and USB both drive the 5 V line.
- Expected runtime with 2000 mAh: roughly 6–8 hours at normal volume.
- Charge through the TP4056's USB-C port (red = charging, green/blue = full).

Simpler alternative: a single "LiPo charger + 5 V boost" board (IP5306
power-bank modules). Fewer parts, but some shut off on their own when the
current drawn is low; test it with the player paused.

## Breadboard → permanent build

1. **Consider swapping the Uno for an Arduino Nano.** Same chip, same pin
   names, same code: just pick "Arduino Nano" as the board when uploading
   (cheap clones may need "ATmega328P (Old Bootloader)"). It's a fraction of
   the size.
2. Solder **female headers** onto perfboard for the Nano, DFPlayer and OLED
   so they can be pulled out and replaced.
3. Run one GND rail and one 5V rail. Keep the speaker wires and the boost
   converter away from the D10/D11 serial lines (less noise in the audio).
4. Solder the capacitor right next to the DFPlayer's VCC pin.
5. Re-run the bench test (below) after every soldering session.

## Case

Measure the finished board first. Openings needed:
- OLED window (1.3" display: active area ~30 x 15 mm)
- 5 buttons (use taller tactile switches or button caps to reach the panel)
- USB-C charging port (TP4056) and power switch
- SD card slot (on the DFPlayer's edge; put it at a wall)
- Speaker grille holes, with the speaker glued or screwed to the wall
- Optionally a hole for the Nano/Uno USB port, for code updates

A 3D-printed box or an off-the-shelf ABS project box (~100 x 60 x 30 mm)
both work.

## Bench test checklist

1. Serial Monitor at 115200: "DFPlayer ok, N tracks", with N = your real
   number of songs.
2. Every button, including wrapping past the first and last track.
3. Let a song end: the next one starts by itself.
4. Pull the SD card while playing, then reinsert: error screen, then recovers.
5. Hold Next: SHUF appears on the top bar; tracks come in random order.
6. Change volume/EQ, wait 3 s, power off and on: settings are kept.
7. On battery at high volume: no resets or OLED flicker (if there are, add
   the capacitor or lower `VOLUME_DEFAULT`).
