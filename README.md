# Sticky Lab

Eigene Firmware für den [Seeed reTerminal Sticky](https://www.seeedstudio.com/) als Werkbank für e-Ink-Technik
und Design: Refresh-Arten, Ghosting, Schriften, Raster, Touch-Rückmeldung und Bedienelemente im Braun-Stil, direkt
auf dem Panel gezeichnet. Ohne Handy-App, ohne BLE. Plan und Entscheidungen: [SPEC.md](SPEC.md), Ergebnisse: [LABORBUCH.md](LABORBUCH.md).

**Sticky-Console:** https://markussstaub-dot.github.io/sticky-lab/console/ – Spiegel, Fernbedienung, Messungen und
Tech-Inventar im Browser. Am PC mit Edge oder Chrome (Web Serial), auf Android mit Chrome (WebUSB, Sticky per
USB-C ans Handy).

## Bedienung
- **KI kurz**: Übersicht mit einer Kachel je Station
- **KI halten**: Bildschirmfoto als PBM nach `/lab/fotos/` auf der microSD
- **▲ / ▼**: blättern (im Refresh-Labor: Refresh-Art wählen)
- **Tippen**: Aktion; Messwerte landen in `/lab/messungen.csv`

## Bauen
Braucht Seeed_GFX2 mit dem Patch aus
[reterminal-sticky/firmware-ble/bibliothek-patch](https://github.com/markussstaub-dot/reterminal-sticky).

```
arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-property build.flash_size=32MB --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
arduino-cli upload -p COMx -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
```

Die Partitionstabelle ist dieselbe wie bei der EDC-Firmware. Beim Umflashen bleiben deren Daten im Flash und auf der
Karte liegen; das Lab schreibt nur nach `/lab/`.

Foto über USB: `python werkzeuge/foto2png.py COMx bild.png`

## Lizenz
MIT
