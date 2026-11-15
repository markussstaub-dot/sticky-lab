// Übersicht: eine Kachel je Station (KI kurz), Tippen springt hin
#pragma once

struct Kachel { const char* nr; const char* name; int station; };   // station < 0 = noch nicht gebaut
static const Kachel KACHELN[8] = {
    {"1", "Refresh", 1}, {"2", "Schrift", -1}, {"3", "Raster", -1}, {"4", "Braun", -1},
    {"5", "Bewegung", -1}, {"6", "Touch", -1}, {"7", "Instrumente", -1}, {"8", "Energie", 8},
};
static const int KW = 170, KH = 160, KX0 = 30, KY0 = 66, KGAP = 20;

static void kachelLage(int i, int& x, int& y) { x = KX0 + (i % 4) * (KW + KGAP); y = KY0 + (i / 4) * (KH + KGAP); }

static void uebersichtBetreten() {
    for (int i = 0; i < 8; i++) {
        int x, y;
        kachelLage(i, x, y);
        const Kachel& k = KACHELN[i];
        if (k.station > 0) ringRR(x, y, KW, KH, 22, 4);
        else ringRR(x, y, KW, KH, 22, 1);
        if (k.station == -2) {
            text("Sticky Lab", x + KW / 2, y + 50, &FreeSansBold12pt7b, MC_DATUM);
            text("FW " LAB_VERSION, x + KW / 2, y + 85, &FreeSans12pt7b, MC_DATUM);
            text("Akku", x + KW / 2, y + 120, &FreeSans9pt7b, MC_DATUM);
            continue;
        }
        text(k.nr, x + KW / 2, y + 62, &FreeSansBold24pt7b, MC_DATUM);
        text(k.name, x + KW / 2, y + 112, &FreeSans12pt7b, MC_DATUM);
        if (k.station < 0) text("kommt noch", x + KW / 2, y + 138, &FreeSans9pt7b, MC_DATUM);
    }
    text("KI = Übersicht   ·   ▲ ▼ = blättern   ·   KI halten = Foto", W / 2, 458, &FreeSans9pt7b, MC_DATUM);
}

static void uebersichtTipp(int x, int y) {
    for (int i = 0; i < 8; i++) {
        int kx, ky;
        kachelLage(i, kx, ky);
        if (drin(x, y, kx, ky, KW, KH) && KACHELN[i].station > 0) { geheZu(KACHELN[i].station); return; }
    }
}
static void uebersichtTaste(Knopf k) {
    if (k == K_DOWN) geheZu(1);   // ▼ führt zur ersten Station
}
