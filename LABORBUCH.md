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
