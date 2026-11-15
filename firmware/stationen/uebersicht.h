// Übersicht: eine Kachel je Station (KI kurz), Tippen springt hin
#pragma once

struct Kachel { const char* nr; const char* name; int station; const char* en; };   // station < 0 = noch nicht gebaut
static const Kachel KACHELN[10] = {
    {"1", "Refresh", 1, "Refresh"}, {"2", "Schrift", 2, "Fonts"}, {"3", "Raster", 3, "Dither"}, {"4", "Braun", 4, "Braun kit"},
    {"5", "Bewegung", 5, "Motion"}, {"6", "Touch", 6, "Touch"}, {"7", "Instrumente", 7, "Gauges"}, {"8", "Energie", 8, "Energy"},
    {"9", "Burgduell", 9, "Castle duel"}, {"10", "Tastatur", 10, "Keyboard"},
};
static const int KW = 142, KH = 160, KX0 = 21, KY0 = 66, KGAP = 12, SPALTEN = 5;

static void kachelLage(int i, int& x, int& y) { x = KX0 + (i % SPALTEN) * (KW + KGAP); y = KY0 + (i / SPALTEN) * (KH + KGAP); }

static void uebersichtBetreten() {
    for (int i = 0; i < 10; i++) {
        int x, y;
        kachelLage(i, x, y);
        const Kachel& k = KACHELN[i];
        if (k.station > 0) ringRR(x, y, KW, KH, 22, 4);
        else ringRR(x, y, KW, KH, 22, 1);
        if (k.station == -2) {
            text("Sticky Lab", x + KW / 2, y + 50, &FreeSansBold12pt7b, MC_DATUM);
            text("FW " LAB_VERSION, x + KW / 2, y + 85, &FreeSans12pt7b, MC_DATUM);
            text(panelChip(), x + KW / 2, y + 120, &FreeSans9pt7b, MC_DATUM);
            continue;
        }
        text(k.nr, x + KW / 2, y + 62, &FreeSansBold24pt7b, MC_DATUM);
        text(EN && k.en ? k.en : k.name, x + KW / 2, y + 112, &FreeSans12pt7b, MC_DATUM);
        if (k.station < 0) text(T("kommt noch", "coming"), x + KW / 2, y + 138, &FreeSans9pt7b, MC_DATUM);
    }
    text(T("KI = Übersicht  ·  ▼ = Station 1  ·  ▲ = English  ·  KI halten = Foto", "AI = overview  ·  ▼ = station 1  ·  ▲ = Deutsch  ·  hold AI = photo"), W / 2, 458, &FreeSans9pt7b, MC_DATUM);
}

static void uebersichtTipp(int x, int y) {
    for (int i = 0; i < 10; i++) {
        int kx, ky;
        kachelLage(i, kx, ky);
        if (drin(x, y, kx, ky, KW, KH) && KACHELN[i].station > 0) { geheZu(KACHELN[i].station); return; }
    }
}
static void uebersichtTaste(Knopf k) {
    if (k == K_DOWN) geheZu(1);   // ▼ führt zur ersten Station
    if (k == K_UP) { EN = !EN; spracheMerken(); geheZu(0); }   // ▲ wechselt die Sprache
}
