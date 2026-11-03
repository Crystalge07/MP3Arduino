# MP3Player build guide

Everything needed to go from parts on a breadboard to a finished,
battery-powered player in a case. Work through it in order; each phase ends
with a check, so problems are caught before they're soldered in.

| Phase | What | Done when |
|---|---|---|
| 0 | [Parts and tools](#0-parts-and-tools) | Everything on hand |
| 1 | [Software setup](#1-software-setup) | Sketch compiles |
| 2 | [SD card](#2-sd-card) | Songs on the card, no hidden files |
| 3 | [Breadboard wiring](#3-breadboard-wiring) | Wired per the table |
| 4 | [Upload and bench test](#4-upload-and-bench-test) | All 10 checks pass on USB power |
| 5 | [Battery power](#5-battery-power) | Plays on battery, charges over USB-C |
| 6 | [Permanent build](#6-permanent-build-perfboard) | Soldered, bench test passes again |
| 7 | [Case](#7-case) | Closed up, everything reachable |

Pin numbers come from `MP3Player/config.h`. If you change a pin there,
change it here too.

---

## 0. Parts and tools

### Electronics

| Part | Qty | Notes |
|---|---|---|
| Arduino Uno, or Arduino Nano for the final build | 1 | Same chip and pins; the Nano is far smaller (see phase 6). |
| DFPlayer Mini | 1 | Clones work; see `DF_USE_ACK` in config.h. |
| 1.3" OLED, 128x64, I2C, SH1106 | 1 | 4 pins: VCC, GND, SCL, SDA. A 0.96" SSD1306 also works (set `OLED_IS_SSD1306 1`). |
| microSD card, 32 GB or smaller | 1 | The DFPlayer doesn't support bigger cards. |
| Speaker, 4–8 Ω, up to 3 W | 1 | A 40 mm 8 Ω 2–3 W speaker is a good fit; 8 Ω draws less battery. |
| Tactile push buttons | 5 | 6x6 mm or 12x12 mm, plus caps if the case wall is thick. |
| 1 kΩ resistor | 1 | DFPlayer RX line. |
| Electrolytic capacitor, 470–1000 µF, ≥ 10 V | 1 | Across DFPlayer power; stops resets and pops. |
| Breadboard and jumper wires | | For phases 3–4. |

### Battery (phase 5)

| Part | Qty | Notes |
|---|---|---|
| 1-cell LiPo, 3.7 V, 1000–2000 mAh, **with built-in protection** | 1 | Check the plug's polarity before connecting anything (see 5.2). |
| TP4056 USB-C charger board, **protected version** | 1 | The one with 6 pads (B+, B-, OUT+, OUT-, IN+, IN-), not 4. |
| MT3608 boost converter (or similar, ≥ 2 A switch rating) | 1 | Turns 3.7 V into 5 V. |
| Slide or toggle switch | 1 | Power on/off. |

### Permanent build (phases 6–7)

| Part | Notes |
|---|---|
| Perfboard, about 70 x 50 mm | Larger if you keep the Uno. |
| Female pin headers | So modules can be unplugged and replaced. |
| Hookup wire, 22–26 AWG | Two colours for 5 V and GND help a lot. |
| Heat-shrink tubing | Over the battery and switch joints. |
| Case | 3D-printed, or an ABS project box about 100 x 60 x 30 mm. |
| M2/M3 screws and standoffs, or hot glue | To hold the boards in the case. |

### Tools

Soldering iron and solder, **multimeter** (required in phase 5), wire
strippers, flush cutters, small screwdriver (boost converter
adjustment), hot glue gun. A drill or file if the case isn't 3D-printed.

---

## 1. Software setup

1. Install the **Arduino IDE 2** (arduino.cc/en/software).
2. Libraries are already installed in `~/Arduino/libraries` (U8g2 and
   DFRobotDFPlayerMini), and the IDE is set to use that folder. On a
   different computer: *Tools → Manage Libraries*, install **U8g2** and
   **DFRobotDFPlayerMini**.
3. Open `MP3Player/MP3Player.ino`.
4. *Tools → Board → Arduino AVR Boards → Arduino Uno* (or *Arduino Nano*;
   cheap Nano clones need *Tools → Processor → ATmega328P (Old
   Bootloader)*).
5. Click **Verify** (✓). It should finish with "Sketch uses … (78%)". If it
   says `U8g2lib.h: No such file`, the libraries are missing; redo step 2.

Command-line alternative:
`arduino-cli compile -b arduino:avr:uno MP3Player`.

### Settings worth knowing (`MP3Player/config.h`)

| Setting | Default | Change it when |
|---|---|---|
| `USE_DISPLAY` / `USE_DFPLAYER` | 1 / 1 | Testing without the OLED or DFPlayer connected. `USE_DFPLAYER 0` fakes 12 eight-second songs and shows "SIM". |
| `VOLUME_DEFAULT` | 20 (of 30) | The board resets at high volume. Only used until a volume has been saved. |
| `LOOP_PLAYLIST` | true | You want it to stop after the last song. |
| `OLED_IS_SSD1306` | 0 | The picture is shifted 2 px with a noisy stripe at one edge. |
| `OLED_I2C_ADDR` | 0x3C | Serial says "no OLED at I2C 0x3C" and the wiring is right: try 0x3D. |
| `TRACK_COUNT_OVERRIDE` | 0 | The DFPlayer reports a wrong track count (some clones). |
| `DF_USE_ACK` | 0 | Only with a genuine DFRobot module. |
| `REMEMBER_SETTINGS` | 1 | You want a fresh start at every power-on. |
| `PLAY_LONG_PRESS` / `NEXT_LONG_PRESS` | 1 / 1 | You don't want hold actions; taps then act on press instead of release. |
| `DEBUG` | 1 | Final build: set 0 for slightly less Serial chatter (errors still print). |

---

## 2. SD card

### Format (once)

1. Insert the card. Open **Disk Utility**, choose *View → Show All
   Devices*, and select the card itself (the top-level device, not the
   volume under it).
2. **Erase**: Name `MP3`, Format **MS-DOS (FAT)**, Scheme **Master Boot
   Record**.
3. Stop macOS from adding its index folder: `mdutil -i off /Volumes/MP3`.

### Add songs

The DFPlayer plays files from a folder called `mp3`, by the number at the
start of the name: `0001.mp3`, `0002.mp3` … up to `3000`. Text after the
4 digits is allowed (`0001_song.mp3`). MP3 and WAV both work.

- **No gaps in the numbering.** The player skips 1–2 missing numbers, but a
  run of 3 or more is treated as the end of the playlist.
- Play order is by number, not by the order the files were copied.

To copy a folder of songs, numbered in alphabetical order:

```sh
mkdir -p /Volumes/MP3/mp3
n=1; for f in ~/Music/player/*.mp3; do
  cp "$f" "/Volumes/MP3/mp3/$(printf %04d $n).mp3"; n=$((n+1))
done
```

(Change `~/Music/player` to wherever your songs are.)

### Every time before ejecting

```sh
dot_clean -m /Volumes/MP3
```

macOS puts a hidden `._` copy next to every file on FAT cards. The DFPlayer
counts those as songs, so the track total comes out too high. The player
corrects itself (Serial shows `WARN: tracks after N missing …`), but a
clean card avoids the hiccup. Then eject properly before pulling the card.

---

## 3. Breadboard wiring

**Unplug USB before changing any wiring.**

| From | To | Notes |
|---|---|---|
| Play button | D2 | Each button's other leg goes to GND. No resistors: the Arduino's internal pull-ups are used. |
| Next button | D3 | |
| Prev button | D4 | |
| Vol+ button | D5 | |
| Vol- button | D6 | |
| DFPlayer TX | D10 | |
| D11 | 1 kΩ resistor, then DFPlayer RX | Without the resistor you get hum or hiss. |
| DFPlayer VCC | 5V | |
| DFPlayer GND | GND | Either of its two GND pins. |
| Capacitor | DFPlayer VCC (+) and GND (−) | As close to the module as possible. **The stripe marks −.** Backwards, it can burst. |
| DFPlayer SPK_1 / SPK_2 | Speaker | **Neither speaker wire goes to GND**: the amp's outputs are both live. |
| OLED SDA | A4 | |
| OLED SCL | A5 | |
| OLED VCC / GND | 5V / GND | **Read the labels on your board**; some swap VCC and GND, and reversed power kills it. |
| A0 | nothing | Leave unconnected on purpose: its electrical noise seeds the shuffle. |

**4-leg buttons:** pairs of legs are joined inside the switch. Use two
**diagonal** legs; they're always on opposite sides of the switch. If
Serial says `WARN: button reads pressed at boot`, that button is wired
across a joined pair.

DFPlayer pins used: one row of 8 is **VCC, RX, TX**, DAC_R, DAC_L,
**SPK_1, GND, SPK_2** (starting from the corner marked VCC); the other row
has a second GND. Go by the labels or the marked corner on your module.

---

## 4. Upload and bench test

1. Plug the Arduino into the Mac. *Tools → Port* → the `usbmodem` /
   `usbserial` entry.
2. Click **Upload** (→). Nothing needs disconnecting: the DFPlayer is on
   D10/D11, not the USB serial pins.
3. *Tools → Serial Monitor*, set to **115200 baud**.

You should see the cassette animation, then the player screen ("READY",
track 1/N). In Serial Monitor:

```
MP3Player booting
[oled] ready
[player] DFPlayer ok, 24 tracks
```

(plus a few other `[player]` lines; with `DEBUG 1` every button press and
track change is logged too).

### Controls

| Button | Tap | Hold 0.7 s |
|---|---|---|
| Play | Play / pause. On an error screen: retry. | Next EQ preset (NORMAL → POP → ROCK → JAZZ → CLASSIC → BASS) |
| Next | Next track | Shuffle on/off ("SHUF" on the top bar) |
| Prev | Previous track | |
| Vol+ / Vol- | Volume 0–30 | |

Taps on Play and Next take effect when you let go (they have to wait to
see whether it's a hold). Any button skips the boot animation.

### Bench test checklist

Run on USB power. Every item must pass before moving to phase 5.

1. [ ] Serial shows `DFPlayer ok, N tracks` and N is your real number of
       songs.
2. [ ] Play starts track 1; Play again pauses; again resumes where it was.
3. [ ] Next/Prev change tracks, including Prev from track 1 (goes to the
       last) and Next from the last (goes to track 1).
4. [ ] Vol+/Vol- move the bar and the sound changes.
5. [ ] Hold Play: the EQ name on the top right changes.
6. [ ] Let a song end: the next one starts by itself.
7. [ ] Hold Next: "SHUF" appears; Next now jumps around randomly. Hold
       again to turn it off.
8. [ ] Pull the SD card while playing: "SD card removed" screen. Reinsert:
       back to the player screen within ~2 s.
9. [ ] Change volume and EQ, wait 3 s, unplug, plug back in: same volume,
       EQ, shuffle setting and track.
10. [ ] At volume 30 for a minute: no resets (the boot animation
        reappearing), no OLED flicker, no crackling.

### Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Screen "DFPlayer silent" | No reply from the DFPlayer: TX/RX swapped, missing 1 kΩ resistor, or no 5 V/GND. Fix, then press Play to retry. |
| Screen "No tracks found" | No card, card not FAT32, or no `/mp3/0001.mp3`. |
| Screen "SD card removed" | Card out or loose. Reinsert; it recovers by itself. |
| Serial `ERROR: no OLED at I2C 0x3C` | SDA/SCL swapped (A4 = SDA), VCC/GND swapped, or the address is 0x3D. The player still works without a screen. |
| Picture shifted 2 px, noisy stripe at one edge | Your OLED is an SSD1306: set `OLED_IS_SSD1306 1`. |
| Serial `WARN: button reads pressed at boot` | That button is wired across two internally joined legs. Use diagonal legs. |
| A button does nothing | Check it in Serial Monitor: each press prints `[btn] NAME`. Nothing printed = wiring. |
| One press counts twice | Worn button: raise `BUTTON_DEBOUNCE_MS` to 50. |
| Board resets at loud volume | Power dip from the speaker. Add/check the capacitor, lower `VOLUME_DEFAULT`, use an 8 Ω speaker. |
| Hum or hiss | Missing 1 kΩ resistor on RX, or a ground loop: try a different USB port/charger or battery power. |
| Track count too high | Hidden files on the card: run `dot_clean -m` (phase 2). |
| Skips a song now and then | Gap in the file numbering, or a corrupt MP3. Re-copy it. |
| Sound but no speaker volume change | Speaker wired to DAC_L/DAC_R instead of SPK_1/SPK_2. |

---

## 5. Battery power

### 5.1 How it fits together

```
          USB-C (charging)
               │
LiPo ── B+/B- [TP4056] OUT+ ── switch ── IN+ [MT3608] OUT+ ── 5V  (Arduino)
                       OUT- ──────────── IN-          OUT- ── GND (Arduino)
```

The DFPlayer and OLED keep getting power from the Arduino's 5V/GND exactly
as on the breadboard.

Power budget: Uno ~50 mA, OLED ~20 mA, DFPlayer ~20 mA idle, speaker
peaks of several hundred mA. With a 2000 mAh battery expect about 6–8
hours at normal volume (more with a Nano, which draws less than an Uno).

### 5.2 Build steps

1. **Check the battery plug's polarity with the multimeter** (DC volts,
   red probe on the red wire): it should read +3.7 to +4.2 V. LiPo
   connectors are not standardised; a reversed battery destroys the
   TP4056. If the colours and reading disagree, trust the reading.
2. Solder the battery to **B+ and B−** on the TP4056.
3. Plug USB-C into the TP4056: red LED = charging, blue/green = full.
   Charge it fully once.
4. Solder TP4056 **OUT+ → switch → MT3608 IN+**, and **OUT− → MT3608 IN−**.
5. **Set the boost voltage, with nothing connected to its output:**
   switch on, put the multimeter on MT3608 OUT+ / OUT−, and turn the small
   brass screw until it reads **5.0 V** (4.9–5.1 is fine). It can take
   10+ turns before the voltage starts moving.
   **Do not skip this.** These boards often ship set to 12 V or more, and
   the Arduino's 5V pin goes straight to the chip with no protection.
6. Switch off. Connect MT3608 **OUT+ → Arduino 5V** and **OUT− → Arduino
   GND**. Use the 5V pin, **not VIN** (VIN needs 7 V or more).
7. Switch on: the player boots. Rerun bench-test items 1, 6 and 10.

### 5.3 Rules for daily use

- **Switch off before plugging the Arduino's own USB port into the Mac**
  (to upload code). Otherwise the boost converter and USB both feed the
  5 V line.
- **Charge through the TP4056's USB-C port**, not the Arduino's. Switch
  the player off while charging so the charger can tell when the battery
  is full.
- The TP4056 charges at 1 A, which is safe for batteries of 1000 mAh and
  up. For a smaller battery, the charge resistor on the board needs
  changing; buy 1000 mAh or more instead.
- When the battery runs low the player resets or goes quiet: recharge.
  The battery's own protection circuit stops it from over-draining.

Simpler alternative: one "LiPo charger + 5 V boost" board (IP5306-based
power-bank modules) replaces the TP4056 + MT3608. Some of these turn
themselves off when little current is drawn; test with the player paused
for a few minutes before relying on one.

---

## 6. Permanent build (perfboard)

### 6.1 Switch to a Nano (recommended)

The Nano has the same chip, pin names and code; it's about a third of the
Uno's size and draws less current. Select *Arduino Nano* in the IDE
(phase 1, step 4) and wire to the same pin labels: D2–D6, D10, D11, A4,
A5, 5V, GND. Upload once on the breadboard to make sure the board works
before soldering anything.

### 6.2 Layout

- Solder **female headers** for the Nano, DFPlayer and OLED; plug the
  modules in afterwards. Nothing gets heat damage, and a dead module is a
  swap, not a rebuild.
- Run one **5 V rail** and one **GND rail** along the board; every module
  takes power from them.
- Put the **capacitor right beside the DFPlayer's VCC pin**.
- Keep speaker wires and the boost converter away from the D10/D11
  wires (less noise in the audio).
- The OLED and buttons will be in the case's front panel, so connect them
  with wires long enough to reach it (leave slack so the case can open).
- The DFPlayer's SD slot must be reachable from outside; plan for it to
  sit at a board edge that will face a case wall.

### 6.3 Order of work

1. Headers and power rails. Check with the multimeter (continuity mode)
   that 5 V and GND aren't shorted to each other.
2. Plug in the Nano only, power it from USB: it should run the upload
   from 6.1.
3. Wire the DFPlayer, resistor, capacitor and speaker. Plug in, test audio.
4. Wire the OLED. Test.
5. Wire the buttons. Test each one.
6. Connect the battery circuit from phase 5.
7. **Run the full bench test again** (phase 4) on battery power.

Test after each step: finding a bad joint is easy when only one thing has
changed.

---

## 7. Case

### Openings

| Opening | Size (approx; measure your parts) |
|---|---|
| OLED window | Visible area about 30 x 15 mm on a 1.3" module (module ~35 x 33 mm) |
| 5 buttons | Button stem diameter + 0.5 mm; use caps if the wall is thick |
| USB-C (TP4056 charging) | ~10 x 4 mm |
| Power switch | Depends on switch |
| SD card slot | ~13 x 3 mm at the DFPlayer edge |
| Speaker grille | Grid of 2–3 mm holes over the speaker |
| Arduino USB (optional) | For code updates without opening the case |

Approximate module sizes for planning: Nano 45 x 18 mm, Uno 69 x 53 mm,
DFPlayer 21 x 21 mm, TP4056 26 x 17 mm, MT3608 37 x 17 mm, 2000 mAh LiPo
about 60 x 35 x 8 mm.

### Assembly

1. Mount the speaker against the grille (hot glue around the rim, or
   screws). A sealed space behind it gives fuller bass.
2. Mount the OLED and buttons on the front panel.
3. Fix the perfboard with standoffs or hot glue, SD slot lined up with its
   opening.
4. Fix the battery with double-sided foam tape, **away from sharp edges
   and screw tips**. Never pierce or crush a LiPo.
5. Before closing: switch on, run bench-test items 2, 4, 6, 8.

---

## 8. Day-to-day

- **Adding songs:** card out, copy with the next numbers (phase 2),
  `dot_clean -m`, eject, card back in. The player re-reads the card by
  itself when it's reinserted.
- **Updating the code:** switch off the battery, plug the Arduino's USB
  into the Mac, upload (phase 4).
- **Saved settings:** volume, EQ, shuffle and the current track are saved
  3 seconds after the last change. Powering off sooner keeps the previous
  values.
