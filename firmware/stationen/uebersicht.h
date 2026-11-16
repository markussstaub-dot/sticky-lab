// Übersicht: eine Kachel je Station (KI kurz), Tippen springt hin
#pragma once

struct Kachel { const char* nr; const char* name; int station; const char* en; };   // station < 0 = noch nicht gebaut
static const Kachel KACHELN[] = {
    {"1", "Refresh", 1, "Refresh"}, {"2", "Schrift", 2, "Fonts"}, {"3", "Raster", 3, "Dither"}, {"4", "Braun", 4, "Braun kit"},
    {"5", "Bewegung", 5, "Motion"}, {"6", "Touch", 6, "Touch"}, {"7", "Instrumente", 7, "Gauges"}, {"8", "Energie", 8, "Energy"},
    {"9", "Burgduell", 9, "Castle duel"}, {"10", "Tastatur", 10, "Keyboard"},
    {"11", "Pins", 11, "Pins"},
};
static const int KACHEL_N = sizeof KACHELN / sizeof KACHELN[0];
// drei Reihen à fünf (seit Station 11, FW 0.9.0); unten bleibt Platz für Hinweis und Pille
static const int KW = 142, KH = 100, KX0 = 21, KY0 = 60, KGAP = 12, SPALTEN = 5;

static void kachelLage(int i, int& x, int& y) { x = KX0 + (i % SPALTEN) * (KW + KGAP); y = KY0 + (i / SPALTEN) * (KH + KGAP); }
// Umschalten auf die andere Firmware (EDC in app0), nur wenn dort etwas Gültiges liegt (platz.h)
static const int PX = 604, PY = 408, PW = 176, PH = 58;

static void uebersichtBetreten() {
    for (int i = 0; i < KACHEL_N; i++) {
        int x, y;
        kachelLage(i, x, y);
        const Kachel& k = KACHELN[i];
        if (k.station > 0) ringRR(x, y, KW, KH, 22, 4);
        else ringRR(x, y, KW, KH, 22, 1);
        if (k.station == -2) {
            text("Sticky Lab", x + KW / 2, y + 26, &FreeSansBold12pt7b, MC_DATUM);
            text("FW " LAB_VERSION, x + KW / 2, y + 54, &FreeSans12pt7b, MC_DATUM);
            text(panelChip(), x + KW / 2, y + 80, &FreeSans9pt7b, MC_DATUM);
            continue;
        }
        text(k.nr, x + KW / 2, y + 36, &FreeSansBold24pt7b, MC_DATUM);
        text(EN && k.en ? k.en : k.name, x + KW / 2, y + 76, &FreeSans12pt7b, MC_DATUM);
        if (k.station < 0) text(T("kommt noch", "coming"), x + KW / 2, y + 94, &FreeSans9pt7b, MC_DATUM);
    }
    text(T("KI = Übersicht  ·  ▼ = Station 1  ·  ▲ = English  ·  KI halten = Foto", "AI = overview  ·  ▼ = station 1  ·  ▲ = Deutsch  ·  hold AI = photo"), 300, 437, &FreeSans9pt7b, MC_DATUM);
    const char* anders = platzAndererName();
    if (anders[0]) {
        char t[24];
        snprintf(t, sizeof t, "> %s", anders);
        pille(PX, PY, PW, PH, t, R_WERKZEUG);
    }
}

static void uebersichtTipp(int x, int y) {
    for (int i = 0; i < KACHEL_N; i++) {
        int kx, ky;
        kachelLage(i, kx, ky);
        if (drin(x, y, kx, ky, KW, KH) && KACHELN[i].station > 0) { geheZu(KACHELN[i].station); return; }
    }
    if (drin(x, y, PX, PY, PW, PH) && platzAndererName()[0]) platzWechseln();   // startet neu in EDC
}
static void uebersichtTaste(Knopf k) {
    if (k == K_DOWN) geheZu(1);   // ▼ führt zur ersten Station
    if (k == K_UP) { EN = !EN; spracheMerken(); geheZu(0); }   // ▲ wechselt die Sprache
}
