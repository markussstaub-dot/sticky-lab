// Hilfe je Station (Markus, 15.11.2026): Tipp auf die Statuszeile oder ▲ und ▼ zusammen zeigt, was die Station
// misst, wie man sie bedient und was die Werte bedeuten. Jeder Tipp und jede Taste schließt sie.
#pragma once

struct HilfeText { const char* titel_de; const char* titel_en; const char* de[6]; const char* en[6]; };

static const HilfeText HILFE[] = {
 { "Übersicht", "Overview",
   { "Elf Stationen loten aus, was der Sticky kann - und ein Geheimnis.", "Kachel antippen = Station öffnen.", "KI kurz = zurück zur Übersicht, KI halten = Bildschirmfoto.",
     "▲ = Deutsch/English, ▼ = Station 1.", "Messwerte landen in /lab/messungen.csv.", "Tipp auf die Statuszeile = Hilfe (überall)." },
   { "Eleven stations probe what the Sticky can do - and one secret.", "Tap a tile = open the station.", "AI short = back to overview, AI hold = screenshot.",
     "▲ = English/Deutsch, ▼ = station 1.", "Measurements go to /lab/messungen.csv.", "Tap the status line = help (everywhere)." } },
 { "Refresh-Labor", "Refresh lab",
   { "Misst, wie lange ein Bildwechsel dauert.", "Unten die Art wählen (Teil, wach, Schnell, Voll), dann Los.", "Das Testfeld kippt bei jedem Lauf; der Probetext wandert.",
     "Sobald alte Reste zu sehen sind: „Reste sichtbar“.", "Teil ~0,9 s, Voll ~2,3 s, Schnell gibt es nicht.", "▲▼ = Art wechseln." },
   { "Measures how long an image change takes.", "Choose the type below (partial, awake, fast, full), then Go.", "The test field flips on every run; the probe text moves.",
     "When you see leftovers: “Ghosting seen”.", "Partial ~0.9 s, full ~2.3 s, fast does not exist.", "▲▼ = change type." } },
 { "Schrift", "Fonts",
   { "Dieselbe Zeile in allen eingebauten Schriften.", "Daneben: Name und Höhe der Großbuchstaben in Pixel.", "Tippen = Lupe 4x um die Stelle, nochmal tippen = zu.",
     "▲▼ = Seite: Sans bzw. Serif und Mono.", "Frage: ab welcher Größe ist es auf Armlänge lesbar?", "" },
   { "The same line in all built-in fonts.", "Next to it: name and cap height in pixels.", "Tap = 4x magnifier around the spot, tap again = close.",
     "▲▼ = page: Sans or Serif and Mono.", "Question: from which size is it readable at arm's length?", "" } },
 { "Raster", "Dither",
   { "Graustufen auf einem Schwarz/Weiß-Panel.", "Seite 1: Graukeil in sechs Verfahren mit Rechenzeit.", "Seite 2: eine beleuchtete Kugel in vier Verfahren.",
     "Tippen = neu rechnen und Zeit messen.", "▲▼ = Seite wechseln.", "Floyd-Steinberg und Atkinson wirken am plastischsten." },
   { "Gray levels on a black/white panel.", "Page 1: gray ramp in six methods with compute time.", "Page 2: a lit sphere in four methods.",
     "Tap = recompute and time it.", "▲▼ = change page.", "Floyd-Steinberg and Atkinson look the most solid." } },
 { "Braun-Baukasten", "Braun kit",
   { "Bedienelemente im Braun-Stil, direkt gezeichnet.", "Seite 1: Knöpfe in Ruhe, gedrückt und aktiv.", "Seite 2: Schieber, Drehknopf (mit dem Finger drehen),",
     "Schalter, Reiter und Tasten wie beim ET66.", "Unten: Zeichenzeit und Refresh-Zeit des letzten Elements.", "▲▼ = Seite wechseln." },
   { "Braun-style controls, drawn directly.", "Page 1: buttons idle, pressed and active.", "Page 2: slider, knob (turn it with your finger),",
     "switch, tabs and keys like the ET66.", "Below: draw and refresh time of the last element.", "▲▼ = change page." } },
 { "Bewegung", "Motion",
   { "Wie viel Animation verträgt e-Ink?", "Balken, Zeiger oder Rolle wählen, dann Los: 12 Bilder.", "Schalter „wach“ = Panel schläft zwischen den Bildern nicht.",
     "Ergebnis: Bilder pro Sekunde und Zeit je Bild.", "Höchstens ~1,2 Bilder/s – Bewegung als Schritte denken.", "▲▼ = Animation wechseln." },
   { "How much animation can e-ink take?", "Pick bar, hand or counter, then Go: 12 frames.", "Switch “awake” = panel does not sleep between frames.",
     "Result: frames per second and time per frame.", "At most ~1.2 frames/s – think of motion as steps.", "▲▼ = change animation." } },
 { "Touch", "Touch",
   { "Zielen: 9 Ziele genau in die Mitte tippen -> Abweichung.", "Rückmeldung: Zeit vom Tippen bis das Bild steht.", "Kritzeln: mit dem Finger zeichnen; Vorlage frei,",
     "Spirale (Abweichung) oder Labyrinth (Zeit, Wände).", "Löschen = leeren. Auch über die Console zeichnen.", "▲▼ = Versuch wechseln." },
   { "Aim: tap the centre of 9 targets -> error.", "Feedback: time from tap until the image is done.", "Scribble: draw with your finger; template free,",
     "spiral (error) or maze (time, walls).", "Clear = empty. You can also draw via the console.", "▲▼ = change test." } },
 { "Instrumente", "Gauges",
   { "Die eingebauten Sensoren als Instrumente:", "Klima mit Taupunkt, Wasserwaage, Uhr, Akku.", "Alle 10 s neu, Tippen = sofort.",
     "Klima landet jede Minute in der CSV (Messreihe).", "„beschlägt gleich“, wenn der Taupunkt nah ist.", "" },
   { "The built-in sensors as gauges:", "climate with dew point, spirit level, clock, battery.", "Every 10 s, tap = now.",
     "Climate is logged every minute (time series).", "“fogging up soon” when the dew point is close.", "" } },
 { "Energie", "Energy",
   { "Was kostet was? Nur ohne Kabel sinnvoll.", "Erst „Ruhe 60 s“ (Grundlast), dann Teil x50, Voll x10,", "dann Schlaf 5 min. KI bricht ab.",
     "Ergebnis: µAh je Bild, mA wach und im Schlaf,", "dazu eine Hochrechnung je Ladung (laut Akku-Chip).", "Links: Akku jetzt; „Werte lesen“ = neu." },
   { "What costs what? Only meaningful unplugged.", "First “Idle 60 s” (baseline), then part. x50, full x10,", "then sleep 5 min. AI key cancels.",
     "Result: µAh per frame, mA awake and asleep,", "plus an estimate per charge (per battery chip).", "Left: battery now; “Read” = refresh." } },
 { "Burgduell", "Castle duel",
   { "Rundenbasiertes Spiel: die andere Burg dreimal treffen.", "Winkel und Pulver mit - / + einstellen, oder ins Feld tippen.", "Feuer = Doppelring. ▲▼ = Winkel fein.",
     "Wind oben in der Mitte, er dreht nach jedem Zug.", "Links: 2 Spieler oder gegen den Sticky.", "" },
   { "Turn-based game: hit the other castle three times.", "Set angle and powder with - / +, or tap the field.", "Fire = double ring. ▲▼ = fine angle.",
     "Wind at the top centre, it changes every turn.", "Left: 2 players or against the Sticky.", "" } },
 { "Tastatur", "Keyboard",
   { "Tippen auf e-Ink: jeder Anschlag klickt sofort,", "die Textzeile frischt gesammelt auf (~0,85 s).", "Übung: den Satz oben abtippen, OK = auswerten.",
     "Ergebnis: Zeichen pro Minute und Fehler.", "DE/EN unten links, ⇧ = ein Großbuchstabe.", "▼ = löschen." },
   { "Typing on e-ink: every key clicks at once,", "the text line refreshes in batches (~0.85 s).", "Practice: type the sentence above, OK = evaluate.",
     "Result: characters per minute and errors.", "DE/EN bottom left, ⇧ = one capital.", "▼ = delete." } },
 { "Pins & Neustarts", "Pins & restarts",
   { "Je Pin: Pegel, Flanken/s (max), Interrupt-Art, Wecken.", "„aus“ = Interrupt abgeschaltet (Sturm > 50 je ms).", "Bewegung, Touch, SD, Laden zählen nur hier; USB per Abfrage.",
     "Unten: Grund dieses Starts, wie lange der vorige lief,", "Neustarts in Folge, letzter Sturm. 3x Absturz = Start hier.", "In CSV = Zeile je Pin, Löschen = Verlauf leeren." },
   { "Per pin: level, edges/s (max), interrupt type, wake.", "“off” = interrupt disabled (storm > 50 per ms).", "Motion, touch, SD, charging count here only; USB polled.",
     "Below: reason for this start, how long the last one ran,", "restarts in a row, last storm. 3 crashes = start here.", "To CSV = one row per pin, Clear = empty history." } },
 { "Fremdsensoren", "External sensors",
   { "Noch nicht gebaut.", "", "", "", "", "" }, { "Not built yet.", "", "", "", "", "" } },
 { "Mesh", "Mesh",
   { "Spricht per Bluetooth direkt mit einem Meshtastic-Knoten (T1000-E).", "▲ oder „Verbinden“ = suchen, koppeln (PIN 123456), lesen.", "Eingehende Texte erscheinen links, Knoten mit Position rechts.",
     "Unten tippen = Schnellantwort senden, ▼ = Kanal wechseln.", "Nur eine BLE-Verbindung: solange verbunden, kommt die Handy-App nicht ran.", "Station verlassen trennt. Verbunden schläft der Sticky nicht." },
   { "Talks to a Meshtastic node (T1000-E) directly over Bluetooth.", "▲ or “Connect” = scan, pair (PIN 123456), read.", "Incoming texts appear on the left, nodes with position on the right.",
     "Tap below = send a quick reply, ▼ = change channel.", "One BLE link only: while connected the phone app can't connect.", "Leaving the station disconnects. Connected, the Sticky stays awake." } },
};
static const int HILFE_N = sizeof HILFE / sizeof HILFE[0];
static bool hilfeAn = false;

static void hilfeZeigen(int station) {
    if (station < 0 || station >= HILFE_N) return;
    const HilfeText& h = HILFE[station];
    hilfeAn = true;
    display.fillScreen(WEISS);
    char t[48]; snprintf(t, sizeof t, "%s  ·  %s", T(h.titel_de, h.titel_en), T("Hilfe", "Help"));
    statuszeile(t);
    for (int i = 0; i < 6; i++) {
        const char* z = EN ? h.en[i] : h.de[i];
        if (z && *z) text(z, 40, STATUS_H + 44 + i * 56, &FreeSans12pt7b, ML_DATUM);
    }
    text(T("Tippen oder Taste = zurück", "Tap or key = back"), W / 2, H - 22, &FreeSans9pt7b, MC_DATUM);
    teil(0, 0, W, H);
}
