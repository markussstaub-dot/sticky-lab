# Sticky Lab

*Deutsch weiter unten.*

A workbench firmware for the [Seeed reTerminal Sticky](https://www.seeedstudio.com/) (ESP32-S3, 3.97″ e-ink
800 × 480, touch, three keys): stations that measure what the little device can do and where e-ink and the small
battery set limits – refresh types and ghosting, fonts, dithering, Braun-style controls drawn on the panel, motion,
touch accuracy and latency, built-in sensors, energy, a castle-duel game, an on-screen keyboard, pins & restarts and
**Mesh**: talking to a Meshtastic node (e.g. SenseCAP T1000-E) directly over BLE, no phone needed. German and
English (switch with ▲ on the overview). No phone app.

**Sticky Console:** https://markussstaub-dot.github.io/sticky-lab/console/ – mirror (actual size, draw on it with
mouse, finger or S Pen), remote control, measurements, tech inventory and **firmware flashing** (this lab and the
whole Seeed Playground) right from the browser. Edge/Chrome on a computer (Web Serial), Chrome on Android (WebUSB).

Findings so far are in [LABORBUCH.md](LABORBUCH.md) (German), e.g.: a partial refresh takes ~0.85–0.95 s almost
regardless of area; there is no fast refresh on the Sticky's SSD1677/SSD2677; animation tops out at ~1.2 frames/s;
the power latch needs **both GPIO45 and GPIO46** high.

## Controls
- **AI short**: overview · **AI hold**: screenshot to `/lab/fotos/` on the microSD
- **▲ / ▼**: page or choose (on the overview: ▲ = language, ▼ = station 1) · **Tap**: action
- Measurements go to `/lab/messungen.csv`

## Build
Needs Seeed_GFX2 with the patch from
[reterminal-sticky/firmware-ble/bibliothek-patch](https://github.com/markussstaub-dot/reterminal-sticky).

```
arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-property build.flash_size=32MB --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
arduino-cli upload -p COMx -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
```
Same partition table as the EDC firmware; its data in flash and on the card stay, the lab only writes to `/lab/`.

## License
MIT

---

## Deutsch

Eigene Firmware für den reTerminal Sticky als Werkbank: Stationen loten aus, was der Kleine kann und wo e-Ink
und der kleine Akku Grenzen setzen – seit 0.10.0 auch, wie er per BLE ohne Handy mit einem Meshtastic-Knoten
spricht (Station 13 „Mesh“). Deutsch und Englisch (▲ in der Übersicht). Plan und Entscheidungen:
[SPEC.md](SPEC.md), Ergebnisse: [LABORBUCH.md](LABORBUCH.md). Die **Sticky-Console** (Link oben) zeigt den Bildschirm
in Originalgröße, steuert den Sticky fern, sammelt Messungen, hat ein Tech-Inventar und spielt Firmware auf – auch
alles aus dem Seeed-Playground.
