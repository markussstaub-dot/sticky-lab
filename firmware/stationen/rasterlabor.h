// Station 3: Raster & Dithering – wie bekommt man Graustufen auf ein Schwarz/Weiß-Panel?
// Seite 1 Graukeil in sechs Verfahren, Seite 2 eine beleuchtete Kugel in vier Verfahren (▲/▼).
// Tippen auf ein Feld zeichnet es neu und misst die Rechenzeit.
#pragma once
#include "../raster.h"

namespace rasterlabor {

static int seite = 0;
static uint32_t us[V_ANZAHL];

// ---- Seite 1: Graukeil ----------------------------------------------------------------------------------
static const int KX = 190, KW = 590, KY0 = 58, KH = 50, KDY = 64;
static uint32_t keil(int i) {
    return dithern(KX, KY0 + i * KDY, KW, KH, (Verfahren)i, [](int x, int) { return x * 255 / (KW - 1); });
}
static void keilZeile(int i, bool mitZeit) {
    int y = KY0 + i * KDY;
    display.fillRect(0, y - 2, KX - 4, KH + 4, WEISS);
    us[i] = keil(i);
    text(V_ANZEIGE(i), 14, y + 14, &FreeSans12pt7b, ML_DATUM);
    char b[24]; snprintf(b, sizeof b, "%lu ms", (unsigned long)(us[i] / 1000));
    if (mitZeit) text(b, 14, y + 38, &FreeSans9pt7b, ML_DATUM);
}

// ---- Seite 2: Kugel -------------------------------------------------------------------------------------
static const int BS = 176, BY = 84;
static const int BX[4] = { 16, 212, 408, 604 };
static const Verfahren BV[4] = { V_SCHWELLE, V_BAYER8, V_FLOYD, V_ATKINSON };
/** Beleuchtete Kugel vor einem senkrechten Verlauf, Licht von links oben */
static uint8_t kugel(int x, int y) {
    float u = (x - BS / 2.0f) / (BS * 0.42f), v = (y - BS / 2.0f) / (BS * 0.42f);
    float r2 = u * u + v * v;
    if (r2 > 1) return 60 + y * 150 / BS;   // Hintergrund: oben dunkler
    float n = sqrtf(1 - r2);
    float licht = (-0.5f * u - 0.6f * v + 0.62f * n);   // Lambert, Lichtvektor normiert
    float spitz = powf(max(0.0f, licht), 30) * 0.6f;     // Glanzpunkt
    float g = 30 + 225 * max(0.0f, licht) * 0.85f + 255 * spitz;
    return (uint8_t)constrain((int)g, 0, 255);
}
static void bild(int i) {
    display.fillRect(BX[i], BY, BS, BS + 50, WEISS);
    us[i] = dithern(BX[i], BY, BS, BS, BV[i], kugel);
    text(V_ANZEIGE(BV[i]), BX[i] + BS / 2, BY + BS + 16, &FreeSans12pt7b, MC_DATUM);
    char b[24]; snprintf(b, sizeof b, "%lu ms", (unsigned long)(us[i] / 1000));
    text(b, BX[i] + BS / 2, BY + BS + 40, &FreeSans9pt7b, MC_DATUM);
}

// ---- Rahmen ---------------------------------------------------------------------------------------------
static void betreten() {
    statuszeile(seite == 0 ? T("3 · Raster: Graukeil", "3 · Dither: gray ramp") : T("3 · Raster: Kugel", "3 · Dither: sphere"));
    if (seite == 0) { for (int i = 0; i < V_ANZAHL; i++) keilZeile(i, true); }
    else {
        text(T("Dieselbe Kugel, vier Verfahren. Tippen = neu rechnen. ▲▼ = Graukeil", "Same sphere, four methods. Tap = recompute. ▲▼ = gray ramp"), W / 2, 64, &FreeSans9pt7b, MC_DATUM);
        for (int i = 0; i < 4; i++) bild(i);
    }
}
static void taste(Knopf) {
    seite = (seite + 1) % 2;
    display.fillScreen(WEISS);
    betreten();
    voll();   // Raster-Vergleich braucht ein sauberes Bild ohne Reste
}
static void tipp(int x, int y) {
    if (seite == 0) {
        int i = (y - KY0 + 7) / KDY;
        if (i < 0 || i >= V_ANZAHL) return;
        keilZeile(i, true);
        teil(0, KY0 + i * KDY - 2, W, KH + 4);
        messung("raster", V_NAMEN[i], us[i], "us", "graukeil 590x50");
    } else {
        for (int i = 0; i < 4; i++) if (drin(x, y, BX[i], BY, BS, BS + 50)) {
            bild(i);
            teil(BX[i], BY, BS, BS + 50);
            messung("raster", V_NAMEN[BV[i]], us[i], "us", "kugel 176x176");
            return;
        }
    }
}

}  // namespace rasterlabor
