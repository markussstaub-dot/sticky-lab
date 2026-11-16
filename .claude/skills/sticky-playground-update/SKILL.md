---
name: sticky-playground-update
description: Neue Sticky-Lab-Version in den Seeed-Playground bringen (Registry-PR „update to an existing firmware“) und die Console-Firmware mitziehen. Nutzen bei „Playground-Update“, „Lab in den Playground“, „neue Version zu Seeed“.
---

# Sticky Playground Update

Bringt eine gebaute Sticky-Lab-Version in die Registry
`Seeed-Projects/reterminal-sticky-playground-registry` (Ordner `firmwares/sticky-lab`) und in die eigene Console.
Erster Lauf: 0.7.6 (PR #126), Update 0.9.1 (PR #132, 16.11.2026).

## Vorher klären
- Welche Version? Ist sie committet und am Gerät gelaufen? Was genau lief am Gerät, steht später ehrlich im
  Testbericht (lieber „0.9.0 am Gerät, 0.9.1 ändert nur …“ als ein geschöntes Häkchen).
- Pille/Plätze: Der Playground spielt nach **app0**. Die Firmware muss dort sinnvoll laufen (`PLATZ_EIGEN` in
  `firmware/platz.h`: außerhalb von app1 neutrale Pille, kein Notausgang).

## Schritte
1. `LAB_VERSION` in `firmware/firmware.ino` hochzählen, bauen:
   `arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-property build.flash_size=32MB --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware`
   Ausgaben: `firmware.ino.bin` (App), `firmware.ino.bootloader.bin`, `firmware.ino.partitions.bin`, `boot_app0.bin`.
2. **Console-Firmware**: `firmware.ino.bin` → `console/fw/sticky-lab/sticky-lab.bin`; in `manifest.json` `version`,
   `size` und `sha256` aller Teile neu setzen. Bootloader/Partitionen/boot_app0 vergleichen (`cmp`), meist gleich.
   SPEC/STAND nachziehen, committen, pushen (Markus fragen, falls nicht schon erlaubt).
3. Registry in den Scratchpad klonen (`--depth 1`), Remote `fork` = `markussstaub-dot/reterminal-sticky-playground-registry`,
   Zweig `sticky-lab-<version>` von upstream `main`.
4. Ordner `firmwares/sticky-lab/firmware/<version>/` mit den vier Dateien (Namen wie bei der Vorversion:
   `bootloader.bin`, `partitions.bin`, `boot_app0.bin`, `sticky-lab.bin`). `npm ci`, dann
   `npm run create:manifest -- sticky-lab <version> --part bootloader.bin@0x0 --part partitions.bin@0x8000 --part boot_app0.bin@0xe000 --part sticky-lab.bin@0x10000`
   **Danach `flashSize` auf `"32MB"` zurücksetzen** (das Werkzeug schreibt 16MB; unsere Partitionen gehen bis 32 MB).
   Den vorgeschlagenen Kanal `stable` nicht übernehmen: `beta`, solange Markus nichts anderes sagt.
5. `firmware.json`: neue Version **zuerst** in `flash.versions` (`manifestPath: firmware/<version>/manifest.json`),
   ältere bleiben. Kurztext ≤ 140 Zeichen, Beschreibung ≤ 800 Zeichen, nur ändern, was die Version ändert.
6. `README.md` der Registry: Stationsliste, „Hardware test record“ (Build, Version, was am Gerät geprüft wurde).
   Burgduell ist ein Geheimnis: als „hidden game“ erwähnen, **nicht verraten, wie es aufgeht**.
7. `npm run validate` muss grün sein. `npm test`: ein PlatformIO-Test scheitert unter Windows an `python3` – bekannt,
   im PR erwähnen.
8. Committen (Autor Markus, noreply-Adresse von GitHub), auf `fork` pushen.
9. PR-Beschreibung nach `.github/pull_request_template.md` (Typ „Firmware: update to an existing firmware“,
   Firmware-only package, SHA-256 der App, Testbericht) **mit dem Write-Werkzeug** in den Scratchpad schreiben
   (ein Heredoc im selben Befehl wie `gh pr create` geht verloren, wenn der Befehl blockiert wird).
10. **Vor `gh pr create` Markus ausdrücklich fragen** – das ist eine Veröffentlichung in seinem Namen. Dann
    `gh pr create --repo Seeed-Projects/reterminal-sticky-playground-registry --head markussstaub-dot:sticky-lab-<version> --base main --title "Update Sticky Lab to <version> (beta)" --body-file <datei>`
11. STAND.md: Version, „wartet: Seeed: PR #… prüfen und mergen“. `gh pr checks <nr> --repo …` zeigt die Prüfungen.

## Nicht tun
- Nichts flashen ohne Auftrag; ein Gerät kann das Lab in app0 haben (reines Lab-Gerät) oder in app1 neben EDC
  (`werkzeuge/lab-nach-app1.ps1`). Vor dem Flashen mit `esptool read_flash 0xe000 0x2000` bzw. `i` über USB prüfen,
  welcher Platz läuft.
- Keine persönlichen Daten in Registry, README oder Testbericht (Namen, Mailadresse, Fotos mit Privatem).
