# Sticky Lab – Spec

**Abgenommen von Markus am 15.11.2026.** Entscheidungen in Abschnitt 7; einzelne „Vorschlag“-Punkte werden beim Bauen entschieden.
Kennzeichnung: **fest** = beschlossen, **Vorschlag** = zur Entscheidung, **offen** = noch zu klären.

Eine eigene Firmware für den reTerminal Sticky (XIAO ESP32-S3, 32 MB Flash, 8 MB OPI-PSRAM, e-Ink 800 × 480 1 bpp,
Touch GT911, Tasten ▲ ▼ KI, microSD, BQ27220, PCF8563, LSM6DS3, SHT40, Summer). Es geht um die Technik des Stickys
und um gutes Design direkt auf dem e-Ink. Es gibt keine Handy-App und keinen EDC-Funktionsumfang.

## 1. Rolle

- fest (Markus, 15.11.2026): **Leitfrage: was kann der Sticky alles, und welche Grenzen setzen e-Ink und der kleine
  Akku?** Jede Station zahlt darauf ein. Der Stand steht im Tech-Inventar der Console (bekannt/gemessen/offen).

- fest: **Werkbank, kein Produkt.** Das Lab erforscht, was das Panel kann, und hält die Ergebnisse fest. Was sich
  bewährt (ein schnellerer Teil-Refresh, ein besserer Knopf, eine lesbarere Schrift), kann später in die EDC-Firmware
  wandern. Das ist dann ein eigener Schritt dort.
- fest: **ohne Handy, ohne BLE.** Alles wird auf dem Sticky gerechnet und gezeichnet.
- Vorschlag: **jeder Versuch misst etwas.** Jede Station zeigt eine Zahl (Millisekunden, Bilder pro Sekunde,
  Treffer in Prozent) und kann sie als Zeile in `/lab/messungen.csv` auf der SD-Karte ablegen. So wird aus „fühlt
  sich schneller an“ ein Wert.
- Vorschlag: **Laborbuch.** `LABORBUCH.md` sammelt die Erkenntnisse mit Datum, z. B. „Teil-Refresh ohne
  Schlafpause 617 ms, blitzt durch“.

## 2. Stationen (was das Lab zeigt)

Jede Station ist eine Seite. Reihenfolge = Bau-Reihenfolge.

| # | Station | Was man sieht und tut | Was gemessen wird |
|---|---|---|---|
| 1 | **Refresh-Labor** | Testmuster (Schachbrett, Text, Fläche), Knöpfe: Voll · Teil · Schnell · Teil ohne Schlaf | Dauer in ms, wie viele Teil-Refreshes bis zum sichtbaren Ghosting, Zähler seit dem letzten Voll-Refresh |
| 2 | **Schrift-Katalog** (umgesetzt FW 0.4.0: Sans, Serif, Mono, TomThumb, Lupe 4×; Strichschrift noch offen) | dieselbe Zeile in allen Schriften und Größen (Bitmap, Strichschrift aus EDC, Ziffernblatt), mit ▲/▼ blättern, Tippen = Lupe 4× | Mindestgröße, die auf Armlänge lesbar ist (Markus entscheidet, das Lab merkt es sich) |
| 3 | **Raster & Dithering** (umgesetzt FW 0.4.0: Graukeil in 6 Verfahren, Kugel in 4; Testbild von SD noch offen) | Graukeil und ein Testbild von der SD: Schwelle, Bayer 4×4/8×8, Floyd-Steinberg, Atkinson, Halbtonpunkte, Linienraster | Rechenzeit; dazu die Frage, was auf 1 bpp am besten aussieht |
| 4 | **Braun-Baukasten** (umgesetzt FW 0.4.0: Knöpfe in 3 Zuständen, Schieber, Drehknopf, Schalter, Reiter, ET66-Tasten; `firmware/braun.h`) | alle Bedienelemente aus `braun.js`, jetzt in C++ in der Firmware gezeichnet: Doppelring, Pille, Rasterrand, dünner Ring, Schieber, Drehrad, Reiter; jeweils Ruhe, gedrückt und aktiv | Zeichenzeit je Element; ob 58 px für „Wurstfinger“ reichen |
| 5 | **Bewegung** (umgesetzt FW 0.4.0: Balken, Zeiger, Zahlenrolle, Schalter „wach“) | Animation per Teil-Refresh: Fortschrittsbalken, Zeiger, Zahlenrolle wie im Kilometerzähler, Ladekreis | erreichte Bilder pro Sekunde; ab wann es ruckelt oder schmiert |
| 6 | **Touch-Labor** (umgesetzt FW 0.3.0: Zielen, Rückmeldung, Kritzeln; 0.5.3: Vorlagen Spirale und Labyrinth) | Zielscheibe mit Treffer-Kreuzen, Kritzelspur, Rückmeldungsarten im Vergleich (Rahmen, Invertieren, Klick, Summer) | Latenz Berührung → sichtbare Rückmeldung, Abweichung vom Ziel in px |
| 7 | **Instrumente** (umgesetzt FW 0.4.0: Klima mit Taupunkt, Wasserwaage, Uhr, Akku; Klima jede Minute in die CSV) | Sensoren als Braun-Instrumente: Klima (SHT40) mit Taupunkt, Wasserwaage (LSM6DS3), Akku mit Spannung, Strom und Verlauf (BQ27220), Uhr (PCF8563) | Werte live, wahlweise alle n Sekunden in die CSV |
| 8 | **Energie** (umgesetzt FW 0.2.0) | Akku-Kapazität aus dem BQ27220 (FullChargeCapacity), Dauerlauf: n Refreshes einer Art, dann Füllstand vorher/nachher; Grundverbrauch wach, Leichtschlaf, Tiefschlaf über Stunden | mAh je Voll-/Teil-Refresh, mA je Zustand, hochgerechnet „Refreshes pro Ladung“ und „Tage im Schlaf“ |
| 9 | **Burgduell** (Markus, 15.11.2026: Spiel lokal statt über das Handy; seit FW 0.9.0 Geheimnis ohne Kachel: ▼ in der Übersicht 2 s halten, sonst `s9`/Console) | Eigenes Artillerie-Spiel (eigene Burgen, eigener Look, kein Nachbau eines bestehenden Spiels): Gelände mit Hügeln, Wind, Zielen per Tipp, ▲▼ Feinwinkel, Feuer = Doppelring; Flugbahn als Punktspur in einem Teil-Refresh, Krater im Gelände, 3 Treffer zerstören eine Burg; 2 Spieler oder gegen den Sticky | Refresh-Zeit je Zug; Vorbild für eine lokale Spielseite in EDC |
| 10 | **Tastatur** (Markus, 15.11.2026, Deutsch und Englisch) | Bildschirmtastatur quer, QWERTZ mit Ä Ö Ü ß bzw. QWERTY, Umschalten DE/EN. Tasten werden nie neu gezeichnet (Rückmeldung = Klick), nur die Textzeile wird gesammelt aufgefrischt, sobald ~0,3 s Pause ist; Anschläge während eines Refreshs gehen dank Touch-Sammler nicht verloren. Übung: vorgegebenen Satz abtippen | Zeichen pro Minute, Fehler (Abstand zum Zielsatz) |
| 11 | **Pins & Neustarts** (Markus, 16.11.2026, nach der Absturzschleife in EDC 0.39.1; umgesetzt FW 0.9.0, noch nicht am Gerät geprüft) | Je Taste und Sensor-Pin (KI, ▲, ▼, Bewegung, Touch-INT, SD-Erkennung, Laden, USB): Pegel live, Flanken pro Sekunde, ob Interrupt bzw. Wecken eingerichtet ist (GPIO-Register); dazu Neustart-Grund der letzten Starts mit einem Zähler, der Neustarts übersteht, und ein Sturm-Melder (welcher Pin wann > 50 Interrupts/ms) | Flanken/s je Pin, Neustarts in Folge mit Grund; Ziel: eine Absturzschleife am Gerät erkennen, ohne USB mitzulesen. Umsetzung: `diagnose.h` (Zähler, Sturmbremse, RTC-Speicher mit Laufzeit des vorigen Starts) + `stationen/pins.h`; Verlauf der letzten 8 Starts, „in Folge“ und Stürme im NVS-Namensraum `lab`; Bewegung/Touch/SD/Laden zählen nur, solange die Station offen ist, USB per Abfrage (ADC-Pin); „in Folge“ fällt nach 60 s Laufzeit auf 0; ab 3 in Folge startet das Lab direkt in Station 11. CSV: `pins,neustart|sturm|stabil|pin-…` |
| 12 | **Fremdsensoren** (Markus, 16.11.2026: geparkt; Idee mit Tom als Messtechniker) | Ein Bastel-Board (Vorschlag: XIAO ESP32 o. Ä.) liest externe Sensoren und funkt kurze Zeilen wie `temp 21.4 °C` an den Sticky – bevorzugt **ESP-NOW** (ohne Router, ohne Koppeln, mehrere Boards), alternativ BLE; auch fertige BLE-Messgeräte (Multimeter, Thermometer, Stromzange). Der Sticky zeigt jeden Kanal automatisch als Braun-Instrument mit Name, Einheit und Verlauf, schreibt CSV auf SD, piept bei Grenzwerten – neue Sensoren brauchen kein Update. Ausbaustufe **Regler**: PID läuft auf dem Board (regelt weiter, wenn Sticky schläft oder Funk weg ist), der Sticky ist Bedienteil: Sollwert per Drehrad, Soll/Ist-Kurve, P/I/D live einstellen, Alarm; Beispiele Sous-vide, Räucherofen, Gärschrank | Funk-Latenz und Reichweite, Akku-Laufzeit als Dauer-Anzeige; Stellglied (Relais, MOSFET) und Sicherheit (Fühlerbruch, Übertemperatur, Netzspannung) sind Toms Teil |
| 13 | **Klopfen & Schritte** (Markus, 16.11.2026: geparkt; Schritte für einen Schrittzähler in EDC) | Der LSM6DS3 erkennt Einfach- und Doppeltipp selbst (Tap-Erkennung im Chip, ESP darf schlafen) und meldet ihn auf INT1 (GPIO7). Die Station zeigt jeden erkannten Tipp, lässt Schwelle und Doppeltipp-Fenster verstellen und hat einen Tascheneintrag: eine Weile mittragen und Fehlauslöser zählen. Prüft auch, ob Klopfen aus dem Leichtschlaf weckt. Dazu der eingebaute Schrittzähler des Chips (zählt selbst, ESP darf schlafen): Zähler live, Probe „100 Schritte gehen“ in der Hand, in der Hosentasche und im Rucksack; prüft, ob Schritte, Klopfen und Wecken bei Bewegung (wie EDC) gleichzeitig laufen | Trefferquote, Fehlauslöser je Stunde, kürzester sicherer Abstand zwischen zwei Tipps, Weckzeit; Abweichung der Schritte je Trageort, Mehrstrom im Schlaf (Energie-Station); Ziel: Doppeltipp als vierte Taste für EDC (z. B. Uhr oder Spickzettel zeigen) und „Schritte heute“ mit Wochenverlauf in EDC (Zähler im Chip bis 65 535 und nach Stromverlust weg: um Mitternacht lesen, merken, zurücksetzen) |
| 14 | **Kälte & Refresh** (Markus, 16.11.2026: geparkt; gedacht als Winternacht auf dem Balkon) | Ein Dauerlauf: alle paar Minuten ein Teil- und ein Voll-Refresh, dazu die Temperatur vom SHT40; den Sticky dafür in den Kühlschrank oder auf den Balkon legen und langsam wieder aufwärmen lassen. Kurve Refresh-Zeit über Temperatur, Hinweis auf sichtbare Reste. **Nachtlauf**: abends raus, morgens rein (Abkühlen und Aufwärmen in einem Lauf); alle 10 min Teil + Voll, dazwischen schlafen; nur auf Akku (unter 0 °C nie laden, Station verweigert den Start mit Kabel bzw. meldet es); morgens Kurve auf dem Display mit tiefster Temperatur, dazu CSV für die Console. Beim Reinholen Gerät im Gefrierbeutel aufwärmen lassen (Beschlagen), Taupunkt aus Station 7 mitschreiben | Teil- und Voll-Refresh in ms je °C, ab welcher Temperatur es zäh wird bzw. Reste bleiben; Akku-Verhalten in der Kälte (laut Akku-Chip) |

- Stand 15.11.2026: alle acht Stationen gebaut (FW 0.4.0). Nächstes: Messungen mit dem Finger und auf Akku, dann
  vertiefen (Strichschrift, Testbild von SD, Ghosting-Serie, „Schnell“-Refresh klären, Summer-Klänge).
- Vorschlag (ursprünglich): zuerst **1, 4, 6** bauen. Sie sind die Grundlage für alles andere: schnelles Zeichnen, die Bauteile und
  das Bediengefühl.
- offen: weitere Stationen, z. B. **Hoch/Quer** (drehen per Lagesensor), **Schlafbild** (was steht nach dem
  Einschlafen), **Summer-Klänge** (Tick, Anschlag, Bestätigung aus der Designsprache).

## 3. Bedienung

- fest (aus EDC übernommen): **dieselben Tastenregeln**: ▲/▼ blättern in dem, was zu sehen ist; Tippen aufs große
  Feld = Hauptaktion; ▲ und ▼ gleichzeitig = Tastenhilfe.
- Vorschlag: **KI kurz = Übersicht** mit einer Kachel je Station, Tippen springt hin (wie das EDC-Menü).
- Vorschlag: **KI halten = Bildschirmfoto** nach `/lab/fotos/` (PackBits wie in EDC). Ein kleines PC-Skript macht
  daraus PNG, damit Ergebnisse im Chat gezeigt werden können. In EDC ist KI halten das Diktat; das gibt es im Lab nicht.
- Vorschlag: **Statuszeile** schlicht: links Stationsname, rechts Refresh-Zähler, Akku und „LAB“, damit man die Geräte
  nie verwechselt.
- fest (aus EDC): Braun-Stil, Knöpfe mindestens ~120 × 58 px, nur Schwarz/Weiß.

## 4. Aufbau

```
sticky-lab/
  SPEC.md · STAND.md · LABORBUCH.md · README.md
  firmware/                 Arduino-Sketch
    firmware.ino            Start, Schleife, Stationswahl
    pins.h                  Pins laut Schaltplan (aus EDC)
    anzeige.h               Seeed_GFX2, Refresh-Arten, Zähler, Zeitmessung
    zeichnen.h              Grundformen auf dem 1-bpp-Puffer (Kreis, Ring, Raster, Text)
    braun.h                 Bedienelemente (C++-Fassung von braun.js)
    schriften.h             Schriften (Strichschrift aus EDC, Bitmap-Schriften)
    raster.h                Dither-Verfahren
    eingabe.h               Tasten und Touch (GT911), Entprellen, Gesten
    sensoren.h              SHT40, LSM6DS3, BQ27220, PCF8563 (aus EDC)
    karte.h                 SD-Karte: /lab/messungen.csv, /lab/fotos, /lab/bilder
    schlaf.h                Schlafen und Wecken (aus EDC)
    station.h               Schnittstelle: name, zeichnen, taste, tipp, tick
    stationen/              eine Datei je Station (refresh.h, schrift.h …)
    partitions.csv          wie EDC (gleiche Aufteilung, siehe 6.)
  werkzeuge/
    foto2png.py             Bildschirmfoto von der SD → PNG
    bauen.ps1               compile + upload in einem
```

- fest: Dateinamen ohne `sd.h` (Kollision mit der SD-Bibliothek), daher `karte.h`.
- Vorschlag: **eine Station = eine Datei**, mit derselben Schnittstelle. Eine neue Station kostet also nur eine
  Datei und einen Eintrag in der Liste.
- Vorschlag: Bauen wie EDC, eigener Build-Ordner:
  `arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc" --build-property build.flash_size=32MB
  --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware`

## 5. Was aus der EDC-Firmware übernommen wird

Als Kopie mit Herkunftsvermerk („aus reterminal-sticky FW 0.31.8“). Es gibt keinen Verweis auf den EDC-Ordner,
denn der bleibt unverändert.

| übernehmen | woher | Anmerkung |
|---|---|---|
| Pins, Tasten aktiv low (KI 4, ▲ 5, ▼ 6), PWR_HOLD 45 **und PWR_LOCK 46** (laut Seeed, fehlt in EDC), Summer 48, Touch 2/3/21/41/42, MISC_I2C 0/1 | EDC `SPEC.md`, `STAND.md` | dazu die Stolperfalle: Tasten und Touch erst **nach** `display.begin()` einrichten |
| Seeed_GFX2 mit Patch `gfxPartialWachHalten` | `firmware-ble/bibliothek-patch` | Refresh-Labor misst genau diese Stellschrauben |
| GT911-Konfiguration nur schreiben, wenn keine da ist | EDC-Firmware | |
| Teil-Refresh, Hoch/Quer-Drehung, 180° | `modus.h`, `widget.h` | |
| PackBits/RLE fürs Bildschirmfoto | `/ist/bildschirm.rle` | |
| Schlaf und Wecken (Tasten, GPIO7 für Bewegung) | `schlaf.h` | |
| Sensoren | `sensoren.h` | Strom-Register 0x0C ist unplausibel, das Lab klärt es (Station 7) |
| Strichschrift, Ziffernblatt | `schrift.h` | |
| Braun-Regeln | `sticky-app/src/braun.js` | wird nach C++ übertragen |
| **nicht** übernommen | | BLE, Abgleich, Bordbuch, Audio/Diktat, Widgets, Fahrtenschreiber, Zettel |

## 6. EDC und Lab auf einem Gerät

Der Sticky hat schon zwei App-Plätze (`app0` und `app1`, je 4 MB), siehe `partitions.csv`.

- fest (15.11.2026, abends geändert): **B mit Umschalten am Gerät** (unten). A bleibt als Rückfall mit dem PC.
- **A: einfach umflashen.** Gerät anstecken, Lab oder EDC aufspielen, je etwa 1 Minute. Kein Aufwand, aber
  immer der PC nötig.
- später **B: zwei Plätze.** EDC bleibt in `app0`, das Lab kommt per esptool nach `app1` (0x410000). Im Lab
  schaltet „zurück zu EDC“ (in der Übersicht) per Neustart auf `app0`. Von EDC zum Lab geht es ohne Änderung an EDC
  nur mit dem PC (Skript `umschalten.ps1`, schreibt nur otadata, wenige Sekunden). Später kann EDC eine Taste
  dafür bekommen, das wäre dann ein Schritt in EDC. Aufwand: etwa ein Abend.
- fest (Markus, 15.11.2026): **B mit Umschalten am Gerät**: EDC in `app0`, Lab in `app1` (beide passen: EDC
  ~1,0 MB, Lab ~0,7 MB, je 4 MB Platz). Lab → EDC: Kachel „zurück zu EDC“ setzt den Start auf `app0` und startet neu.
  EDC → Lab: braucht eine kleine Ergänzung in EDC (Menüeintrag „Sticky Lab“, setzt `app1`). Daten bleiben getrennt.
  - Vorschlag **Notausgang**: beim Einschalten ▲ halten startet den jeweils anderen Platz – falls eine Firmware hängt
  - Vorschlag **feste Plätze**: EDC immer `app0`, Lab immer `app1`. Ein Update (Kabel, Karte oder BLE) schreibt nur in
    den Platz seiner eigenen Firmware, nie „in den freien“ – sonst überschreibt ein EDC-Update das Lab
  - Vorschlag: der Platz, der gerade läuft, steht in der Übersicht bzw. im EDC-Menü (z. B. „EDC · Platz 0“)
  - Lab 0.9.1 (16.11.2026, für den Seeed-Playground): läuft das Lab nicht in seinem Platz `app1` (Playground spielt
    nach `app0`), heißt die Pille neutral „> app1“ und der Notausgang beim Einschalten ist aus – sonst stünde dort
    „> Sticky Lab“ für ein unbekanntes Abbild (z. B. Rest der Werks-Firmware). Schalter `PLATZ_EIGEN` in `platz.h`;
    EDC braucht ihn nicht (ohne Schalter wie bisher), beim nächsten Abgleich der Datei aber mitnehmen.
  - umgesetzt Lab 0.8.0 + EDC 0.33.0 (15.11.2026, gebaut, noch nicht geflasht): `firmware/platz.h` (gleich in EDC),
    Pille „> EDC“ unten rechts in der Übersicht (nur wenn in `app0` etwas Gültiges liegt), Notausgang ▲ 1 s beim
    Einschalten. Aufspielen nach `app1`: `werkzeuge\lab-nach-app1.ps1 -Port COMx` (baut, prüft Größe, schreibt
    nur 0x410000; EDC bleibt Startplatz)
- Vorschlag (Markus, 15.11.2026): **Firmware-Bibliothek in der APK**: die Sticky-App trägt fertige Firmwares (EDC,
  Lab, später Spiele) mit und spielt die gewählte per BLE in den freien App-Platz (OTA), danach Umschalten wie oben.
  ~0,7–1 MB über BLE dauert grob 2–5 min. Braucht OTA-Empfang in EDC und eine Auswahlseite in der App – Arbeit im
  EDC-Projekt, nicht im Lab.
- fest (egal ob A oder B): **das Lab rührt die EDC-Daten nicht an**: kein Formatieren der `spiffs`-Partition
  (Bordbuch-Seiten), eigener NVS-Namensraum `lab`, auf der SD nur der Ordner `/lab/`.
- Hinweis: ein normales `arduino-cli upload` der EDC-Firmware schreibt `app0` und stellt den Start auf `app0`. Das Lab
  in `app1` bleibt dabei liegen.

## 7. Entscheidungen (Markus, 15.11.2026)

1. fest: **Lab-Gerät ist vorerst der zweite Sticky**, der erste bleibt bei der EDC-Firmware. Geht das Lab-Gerät
   später zurück auf EDC, wird `/lab/` von der SD gelöscht. Deshalb gilt besonders: Das Lab lässt die EDC-Daten auf
   dem Gerät in Ruhe (siehe 6.).
2. fest: **zwei Plätze mit Umschalten am Gerät (B)** (abends geändert, vorher A: Umflashen). Damit geht z. B. der
   Multiclient-Test mit zwei EDC-Stickys, ohne das Lab zu verlieren.
3. fest: **eigenes Repo `sticky-lab`, öffentlich.** Anlegen und pushen erst nach Markus' ausdrücklichem „Ja“. Keine
   persönlichen Daten, auch nicht in Testbildern oder Messungen.
4. fest: **Lizenz MIT** wie reterminal-sticky.
5. Vorschlag: als Erstes die Stationen **1 Refresh, 4 Braun, 6 Touch**, danach in der Reihenfolge der Tabelle.

## 8. Sticky-Console auf dem Zenbook (Markus, 15.11.2026)

- fest: **nur fürs Lab**, EDC bleibt außen vor.
- fest: **eine einzelne HTML-Seite** (`console/index.html`), im Edge/Chrome auf dem Zenbook geöffnet, spricht per **Web Serial** über
  USB mit dem Sticky. Keine Installation, kein Server. Läuft auch auf dem ScreenPad unten.
- fest (umgesetzt 15.11.2026): Reiter **Steuerung** und **Tech-Inventar** („was bietet der Kleine“: Bauteile, Zahlen,
  Grenzen, offene Fragen; Refresh-Zeiten laufen live aus den Messungen ein).
- Inhalt Steuerung (umgesetzt): Live-Spiegel (Bildschirmfoto per „f“, auf Knopfdruck oder alle paar Sekunden), Stationen
  wählen, ▲/▼/Los fernbedienen, Messungen als Tabelle mit Verlauf, serielles Log, Fotos speichern.
- fest (umgesetzt 15.11.2026): **Console auch auf dem Fold** – Chrome auf Android spricht per WebUSB direkt mit dem
  CH343 (CDC-ACM, 115200, DTR+RTS an), eigener kleiner Treiber in der Console. Am Fold geprüft: Verbinden und
  Foto holen gehen. Für Energie-Messungen ungeeignet (Fold speist den Sticky).
- fest (15.11.2026): Seite für unterwegs über **GitHub Pages** (https://markussstaub-dot.github.io/sticky-lab/console/).
  Am Rechner geht sie weiter als Datei oder über localhost.
- fest (Markus, 15.11.2026): **Reiter „Firmware“ in der Console**: liest das Seeed-Playground-Verzeichnis
  (github.com/Seeed-Projects/reterminal-sticky-playground-registry, `firmwares/*/firmware.json` und Manifeste im
  ESP-Web-Tools-Format) und das eigene Sticky Lab (liegt mit Manifest auf GitHub Pages), Aufspielen mit esptool-js
  über Web Serial direkt aus dem Browser. Ganze Abbilder ab Adresse 0 überschreiben auch Flash-Daten (Bordbuch,
  Einstellungen); die SD-Karte bleibt. Am Fold vorerst nur Liste, Flashen am Rechner.
- Vorschlag: Flashen sonst weiter mit arduino-cli. Die Console gibt den Port frei („Trennen“), damit hochgeladen werden kann.
- Vorschlag: Die Firmware antwortet auf USB maschinenlesbar (Zeilen wie `MESSUNG …`, `STATION …`, `FOTO n` + Daten).

## 9. Ausbaufähig bleiben, z. B. für einen farbigen Sticky (Markus, 15.11.2026)

- fest: **Das Lab hängt nicht an einem Panel.** Alles Panel-Abhängige steht in einem **Panel-Profil**
  (`firmware/anzeige.h`: Name, Breite, Höhe, Anzahl Farben, Pixeldichte, kann Teil-Refresh). Umgesetzt FW 0.3.0;
  der Sticky meldet das Profil per `INFO`, die Console richtet Spiegel und Originalgröße danach aus.
- Vorschlag: **Farbe über Rollen, nicht über feste Werte.** Zeichnen mit Rollen wie heute (Hauptaktion, Auswahl,
  Werkzeug, Nebensache) plus einer Rolle „Akzent“. Auf Schwarz/Weiß wird Akzent zu Raster, auf Farbe zu einer
  echten Farbe (Braun-Vorbild: ein einziges Orange/Grün als Signal).
- Vorschlag: **Stationen fragen Fähigkeiten ab** statt sie anzunehmen: kein Teil-Refresh → Refresh-Labor misst nur
  Voll, Touch-Rückmeldung zeigt das. Neue Stationen für Farbe: Farbtafel, Farb-Dithering, Refresh-Dauer je Farbe.
- Vorschlag: **Foto mit Farbe**: heute PBM (1 bpp); für Farbe ein indiziertes Format (z. B. 4 bpp + Palette) und die
  Console zeichnet mit der Palette aus dem Profil.
- Vorschlag: **ein Build je Gerät** über ein Compile-Flag (`-DPANEL_STICKY_SW`, später `-DPANEL_STICKY_FARBE`);
  Pins und Treiber je Board im Profil. Der Rest des Labs bleibt gleich.
- offen: welches Farbpanel (Spectra 6 o. ä.), ob Seeed_GFX2 es unterstützt, ob Touch und Tasten gleich bleiben.

## 10. Offen

- Hinweis Station 12: die USB-Buchse hängt am Wandler CH343, der Sticky ist dort immer Gerät, nie USB-Host – externe
  Hardware also per Funk anbinden (ESP-NOW/BLE), nicht per USB-Kabel.

- Vorschlag (16.11.2026, aus Station 11): **EDC zeigt eine Neustartschleife selbst an** – startet er mehrmals in
  Folge nach Absturz/Watchdog neu, steht auf Home „Neustartschleife, Grund …“ statt stillem Blinken. Anlass:
  EDC 0.39.1 hing nach dem Wechsel aus dem Lab in einer Schleife (Tasten-Interrupt-Sturm); behoben in EDC 0.39.2
  durch Zurücksetzen der Tasten-Pins beim Start + Sturmbremse. Im Lab nachgezogen mit FW 0.9.0 (16.11.2026):
  `tastenInit()` wie EDC 0.39.2, Sturmbremse in `diagnose.h`, dazu Station 11.

- Idee (Markus, 15.11.2026): **Sticky Stack** – zwei Stickys halten magnetisch aneinander, aber nur Rücken an
  Front (gestapelt). Nebeneinander (1600 × 480) bräuchte einen gedruckten Halter. Braucht eine Verbindung zwischen den Geräten (BLE oder
  ESP-NOW); geparkt.

- Ab wann ist ein Ergebnis reif für EDC, und wer trägt es dorthin? Vorschlag: Markus entscheidet je Fund, eine
  EDC-Session baut es ein.
