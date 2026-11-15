// Übersicht: eine Kachel je Station (KI kurz), Tippen springt hin
#pragma once

struct Kachel { const char* nr; const char* name; int station; };   // station < 0 = noch nicht gebaut
static const Kachel KACHELN[10] = {
    {"1", "Refresh", 1}, {"2", "Schrift", 2}, {"3", "Raster", 3}, {"4", "Braun", 4},
    {"5", "Bewegung", 5}, {"6", "Touch", 6}, {"7", "Instrumente", 7}, {"8", "Energie", 8},
    {"9", "Burgduell", 9}, {"", "Info", -2},
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
        text(k.name, x + KW / 2, y + 112, &FreeSans12pt7b, MC_DATUM);
        if (k.station < 0) text("kommt noch", x + KW / 2, y + 138, &FreeSans9pt7b, MC_DATUM);
    }
    text("KI = Übersicht   ·   ▲ ▼ = blättern   ·   KI halten = Foto", W / 2, 458, &FreeSans9pt7b, MC_DATUM);
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
}
