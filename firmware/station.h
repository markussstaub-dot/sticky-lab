// Schnittstelle einer Station: eine Datei je Station unter stationen/, Eintrag in STATIONEN (firmware.ino)
#pragma once

struct Station {
    const char* name;                    // Statuszeile und Kachel
    void (*betreten)();                  // ganze Seite in den Puffer zeichnen (Voll-Refresh macht der Rahmen)
    void (*taste)(Knopf k);              // ▲/▼ (KI behandelt der Rahmen: kurz = Übersicht, halten = Foto)
    void (*tipp)(int x, int y);          // Finger aufgesetzt
    void (*tick)();                      // in jeder Schleife, darf nil sein
};

static void geheZu(int station);         // firmware.ino
