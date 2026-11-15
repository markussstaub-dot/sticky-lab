# Laborbuch

Erkenntnisse mit Datum. Rohdaten: `/lab/messungen.csv` auf der SD-Karte bzw. CSV aus der Console.

## 15.11.2026 – FW 0.1.0 auf dem Lab-Gerät (zweiter Sticky), 80 MHz, am USB

| Refresh | Fläche | Dauer | Bemerkung |
|---|---|---|---|
| Teil | 368 × 300 px (+2 Rand) | 951–952 ms (3×) | sehr gleichmäßig |
| Teil wach (`gfxPartialWachHalten`) | dieselbe | 853–854 ms (2×) | spart ~100 ms; EDC maß für Kritzel 840 → 742 ms |
| Voll | ganzes Bild | 2277 ms | |
| Schnell (`updateFast`) | ganzes Bild | **0 ms** | kehrt sofort zurück; ob und wie das Panel umschaltet, ist offen (EDC sah im Bordbuch falsche Bilder) |

- Teil-Refresh einer etwas größeren Fläche dauert länger als in EDC (952 statt 840 ms). Vermutung: die Dauer
  hängt von der Fläche ab. Das prüft eine Messreihe mit verschiedenen Flächen.
- Die FreeSans-Schriften kennen nur ASCII. Umlaute (Grundbuchstabe + Punkte), Mittelpunkt und ▲▼ setzt `text()`
  selbst; am Gerätefoto sauber.
- Der GT911 auf dem Lab-Gerät hat seine eigene Konfiguration 0x41, Touch geht ohne Schreiben.

## 15.11.2026 – FW 0.2.0: Station 8 Energie

- **Akku-Chip BQ27220 meldet 7500 mAh** für Design, voll und Rest, dazu 0 Zyklen und 100 % Gesundheit, Strom 0 mA
  am USB bei vollem Akku. Das sieht nach einem Chip mit Werkseinstellung aus, der nie auf die echte Zelle eingestellt
  wurde. Das erklärt wohl die seltsamen Prozent- und Stromwerte aus EDC (z. B. „lädt mit 3–5 A“, Prozent nach
  Tiefentladung). Folge: Prozent und Hochrechnung sind nur „laut Chip“. Ob wenigstens der Strom stimmt, ist offen.
- Nächste Schritte: echte Kapazität herausfinden (Datenblatt/Seeed, oder einmal voll → leer mit bekannter Last messen,
  dabei die Spannung loggen); Stromwert gegenprüfen (z. B. USB-Messgerät beim Laden).
- Die Messläufe brauchen Akkubetrieb (Kabel ab). Am USB zeigt „Ruhe 60 s“ 0,0 mA, weil das Netzteil liefert.

## 15.11.2026 – Console am Fold
- Chrome 157 auf dem Fold6 hat `navigator.serial` und `navigator.usb`. Die Console nimmt auf Android WebUSB und
  spricht den CH343 (1A86:55D3) als CDC-ACM an. Verbinden, Spiegel und „Foto holen“ gehen (Markus, am Fold getestet).

## 15.11.2026 – FW 0.3.0: Station 6 Touch

- **Teil-Refresh kostet fast unabhängig von der Fläche ~0,85–0,95 s:** Punkt 28 × 28 px 838 ms, Feld 180 × 150 px
  950 ms (Rahmen) bzw. 1001 ms (invertiert), Testfeld 368 × 300 px 952 ms. Die Zeit steckt im Ablauf des Panels
  (Wellenform), nicht in der Datenmenge. Folge fürs Design: Rückmeldung per Bild ist nie schneller als ~0,85 s;
  sofortige Rückmeldung geht nur über Klick (0 ms) – oder später über die LED.
- Zielen- und Rückmeldungs-Logik per USB geprüft (künstliche Tipps mit Versatz +3/−2 px → 3,6 px gemessen).
  Echte Finger-Messung und Kritzeln stehen noch aus.
- Ohne Kabel noch nicht gemessen (Protokoll auf der Karte leer).

## 15.11.2026 – FW 0.4.0: Stationen 2, 3, 4, 5, 7

- **Animation: höchstens ~1,2 Bilder/s.** Je Bild ein Teil-Refresh des bewegten Bereichs: Balken 859 ms (normal)
  bzw. 761 ms (wach), Zeiger 784 ms, Zahlenrolle 782 ms (beide wach). „wach“ spart ~100 ms je Bild, wie in EDC.
  Folge: Bewegung auf dem Sticky als Schrittfolge denken (Zeiger springt, Rolle zählt), nicht als Film.
- **Zeichnen kostet merklich bei 80 MHz:** Pille ~42 ms, Schieber ~66 ms, Reiter ~67 ms, Drehknopf ~133 ms
  (schräge breite Linien), alle vier Instrumente ~0,6 s. Dithering 590 × 50 px ~0,1 s, Kugel 176 × 176 ~0,3 s.
  Hauptkosten sind einzelne drawPixel-Aufrufe; schneller ginge es direkt in den 1-bpp-Puffer oder mit 240 MHz.
- **Dithering:** Floyd-Steinberg und Atkinson zeigen die Kugel am plastischsten; Atkinson kontrastreicher (gibt nur
  6/8 des Fehlers weiter, dunkle Bereiche saufen ab). Bayer 8×8 ist gleichmäßig, aber sichtbar gerastert.
- **Sensoren laufen:** SHT40 24,3 °C / 37 % (Taupunkt 8,6 °C, im Gehäuse am USB, Eigenwärme noch offen),
  LSM6DS3TR-C liefert nach dem Einschalten erst nach ~30 ms Werte, Uhr PCF8563 stimmt (von EDC gestellt).
- **Schrift:** `textWidth()` von Seeed_GFX2 lässt ein Leerzeichen am Ende weg; die Umlaut-Punkte saßen deshalb
  eine Leerzeichenbreite zu weit links. Jetzt eigene Breite aus den Glyphen-Vorschüben.

## 15.11.2026 – „Schnell“ geklärt

- Seeed_GFX2 wählt für den Sticky beim Start zwischen **SSD1677 und SSD2677** (je Gerät zufällig verbaut, erkannt
  über Befehl 0x70). Keiner der beiden Treiber meldet `supportsFastRefresh()`, darum liefert `updateFast()` sofort
  „NotSupported“ und frischt **gar nicht** auf. Die 0 ms im Refresh-Labor heißen also „kein Refresh“. EDCs
  „falsches Bild beim schnellen Modus“ passt dazu (es blieb das alte Bild stehen).
- Offen: einen eigenen schnellen Modus (kürzere Wellenform/LUT) für SSD1677/SSD2677 bauen und messen.

## 15.11.2026 – Akkubetrieb: das Lab-Gerät hält den Strom nicht

- Ohne Kabel startet das Gerät nur, solange KI (zugleich Power-Taste) gedrückt ist; beim Loslassen ist es aus
  (Protokoll: „start … akku“, danach kein einziges Lebenszeichen; lässt man mitten im Voll-Refresh los: Streifen).
- **Gegentest mit der EDC-Firmware 0.31.8: genauso.** Also kein Fehler der Lab-Firmware, sondern dieses Gerät:
  die Selbsthaltung über PWR_HOLD (GPIO45 HIGH) wirkt hier nicht. Beim anderen Sticky klappt dieselbe Firmware.
- Die Werks-Firmware meldet „HAL init done (power latched inside)“ – die Selbsthaltung ist also vorgesehen. Offen:
  ob die Werks-Firmware auf diesem Gerät hält (anderer Pin, andere Polarität, oder Hardwarefehler).
- Mit EDC am USB läuft alles; **beim Abziehen ist es sofort aus** (Streifen, wenn gerade ein Refresh lief bzw. das
  Panel ohne sauberes Abschalten den Strom verliert). Die Firmware kommt gar nicht zum Halten.
- Folge: Energie-Messungen (Station 8) gehen auf diesem Gerät erst, wenn das geklärt ist.

## 15.11.2026 – gelöst: zweiter Haltepin GPIO46 (FW 0.4.2)

- Seeeds offizielle Beispiele (github.com/Seeed-Projects/sticky-esp-idf-peripherals, `main/pin_config.h`) nennen
  **zwei** Haltepins: `PIN_POWER_HOLD 45` und **`PIN_POWER_LOCK 46`**; `demo_board_init()` setzt beide als Erstes
  auf HIGH. EDC (und das Lab bis 0.4.1) setzte nur GPIO45.
- Mit GPIO46 HIGH bleibt das Lab-Gerät beim Abziehen an, Tasten klicken (Markus, getestet). Im Leichtschlaf wird
  GPIO46 wie GPIO45 per `gpio_hold_en` festgehalten.
- Warum der andere Sticky ohne GPIO46 hält, ist offen (Platinen-Version, oder der Pin landet dort zufällig HIGH).
- **Folge für EDC:** dieselbe Zeile gehört in die EDC-Firmware, sonst geht ein Gerät wie dieses ohne Kabel aus.
- Nebenfund im Seeed-README: eine SHT40-Messung beim Start setzt die Wellenform-Temperatur des Panels.

## 15.11.2026 – FW 0.4.3: welcher Display-Controller?

- Die Firmware meldet jetzt beim Start den von Seeed_GFX2 erkannten Controller (`system,panel` in der CSV, `INFO`,
  Statuszeile der Übersicht, Tech-Inventar).
- Beide Geräte: **SSD1677**, Treiber meldet **Graustufen-Refresh mit 4 Stufen: ja**. Der GPIO46-Unterschied hat
  also nichts mit der Display-Variante zu tun; am ehesten der unbestimmte Pegel des Strapping-Pins GPIO46.
- Neue Frage fürs Lab: echte Graustufen (4 Stufen, `updateGray`) statt Dithering – wie sieht es aus, wie lange
  dauert es? Kandidat für Station 3.

## 15.11.2026 – Beobachtung: Sticky Stack

- Zwei Stickys **halten magnetisch aneinander** (Magnete im Gehäuse). Idee fürs Lab, geparkt: „Sticky Stack“ –
  zwei Geräte nebeneinander als ein breites Bild (1600 × 480) oder Rücken an Rücken als doppelseitige Anzeige.
  **Nachtrag (Markus):** die Magnete halten nur Rücken an Front – gestapelt, das untere Display ist verdeckt.
  Nebeneinander und Rücken an Rücken gehen also nicht magnetisch. Bleibt: Stapel als „Paket“ (oben zeigt, unten
  schläft) oder ein gedruckter Halter für nebeneinander. Offen: ob die Magnete Touch oder Sensoren stören.

## 15.11.2026 – erste Energie-Messung auf Akku (FW 0.4.3)

- **Leichtschlaf: 6,1 mA** (30 × 10 s, Strom nach jedem Wecken gelesen). Hochgerechnet ~ 2 Tage pro 300 mAh – die
  echte Kapazität ist weiter offen (Chip meldet 7500 mAh).
- Ruhe/Teil/Voll waren unbrauchbar: im Sammler lief `int16 × uint32` über (negativer Entladestrom → +4,3 Mio. je
  Probe). Behoben in 0.4.4, Messung wiederholen.
- Akku-Chip nach ~17 min Akkubetrieb: Rest 7433 von 7500 mAh (−67 „mAh“ laut Chip, nicht verlässlich).

## 15.11.2026 – FW 0.5.0/0.5.1: Burgduell, Touch-Sammler

- **Station 9 Burgduell** (eigenes Spiel, lokal): ein Zug = ein Teil-Refresh des ganzen Bilds, **1204 ms**. Flugbahn
  als Punktspur, Krater, 3 Treffer; der Sticky als Gegner probiert alle Kräfte durch und streut anfangs, mit jedem
  Zug weniger. Markus: „sieht toll aus“.
- **Kritzeln war zackig** (Markus): der Touch wurde nur in `loop()` abgefragt, während jedes Teil-Refreshs (~0,85 s)
  fehlten Punkte → lange gerade Striche. Jetzt fragt eine eigene Aufgabe den GT911 alle 15 ms ab (wie in Seeeds
  Beispielen) und sammelt in einer Warteschlange (256 Ereignisse); die Linie bekommt alle Punkte. Mit dem Finger
  noch zu prüfen.
- Nebenbei: Statuszeile zeigte nach dem Akkutest kurz **3 %** (Akku-Chip, unkonfiguriert – Prozent bleibt fraglich).

## 15.11.2026 – FW 0.5.3: Vorlagen beim Kritzeln, Firmware-Reiter

- Kritzeln hat Vorlagen (Pille wechselt frei → Spirale → Labyrinth, Idee Markus): **Spirale** nachfahren misst die
  mittlere Abweichung in px/mm; **Labyrinth** (bei jedem Start neu gewürfelt, 21 × 8 Zellen à 36 px) misst Zeit
  vom Ring zum Punkt und Wandberührungen. Mit dem Finger noch zu messen.
- Console: Reiter **Firmware** listet das eigene Lab und alle 37 Playground-Firmwares (36 aufspielbar) und spielt
  per esptool-js über Web Serial auf. Im Browser geprüft: Liste lädt; Aufspielen braucht Markus' Klick.
