// Station 2: Schrift-Katalog – dieselbe Zeile in allen eingebauten Schriften und Größen
// Seite 1 Sans, Seite 2 Serif und Mono (▲/▼). Tippen = Lupe 4× um die Stelle (noch mal tippen = zu).
// Frage fürs Lab: ab welcher Größe ist was auf Armlänge lesbar, wie wirken Serifen auf 1 bpp?
#pragma once
#include <font/GFXFF/FreeSans18pt7b.h>
#include <font/GFXFF/FreeSans24pt7b.h>
#include <font/GFXFF/FreeSerif9pt7b.h>
#include <font/GFXFF/FreeSerif12pt7b.h>
#include <font/GFXFF/FreeSerif18pt7b.h>
#include <font/GFXFF/FreeSerif24pt7b.h>
#include <font/GFXFF/FreeMono9pt7b.h>
#include <font/GFXFF/FreeMono12pt7b.h>
#include <font/GFXFF/FreeMonoBold18pt7b.h>
#include <font/GFXFF/TomThumb.h>

namespace schriften {

struct Eintrag { const char* name; const GFXfont* f; };
static const Eintrag SEITE1[] = {
    { "Sans 9", &FreeSans9pt7b }, { "Sans 12", &FreeSans12pt7b }, { "Sans 18", &FreeSans18pt7b },
    { "Sans 24", &FreeSans24pt7b }, { "Sans fett 12", &FreeSansBold12pt7b }, { "Sans fett 24", &FreeSansBold24pt7b },
};
static const Eintrag SEITE2[] = {
    { "Tom Thumb", &TomThumb }, { "Serif 9", &FreeSerif9pt7b }, { "Serif 12", &FreeSerif12pt7b },
    { "Serif 18", &FreeSerif18pt7b }, { "Serif 24", &FreeSerif24pt7b }, { "Mono 9", &FreeMono9pt7b },
    { "Mono 12", &FreeMono12pt7b }, { "Mono fett 18", &FreeMonoBold18pt7b },
};
static const char* PROBE = "Grundfaulheit 0123 Äpfel";
static int seite = 0;
static bool lupeAn = false;

static void seiteZeichnen() {
    const Eintrag* e = seite == 0 ? SEITE1 : SEITE2;
    int n = seite == 0 ? 6 : 8;
    int y = STATUS_H + 8;
    for (int i = 0; i < n; i++) {
        int zh = e[i].f->yAdvance;
        int cap = -e[i].f->glyph['H' - e[i].f->first].yOffset;
        char b[40];
        snprintf(b, sizeof b, "%s · %d px", e[i].name, cap);
        text(b, 12, y + zh / 2, &FreeSans9pt7b, ML_DATUM);
        text(PROBE, 170, y + zh / 2, e[i].f, ML_DATUM);
        y += max(zh, 22) + (seite == 0 ? 6 : 2);
    }
    text("Tippen = Lupe 4x  ·  ▲▼ Seite", W - 12, H - 14, &FreeSans9pt7b, MR_DATUM);
}

// Lupe: Ausschnitt 100 × 60 px um den Tipp, 4-fach, in einem Kasten auf der anderen Bildhälfte
static const int LW = 100, LH = 60, LF = 4;
static uint8_t* lupeQuelle = nullptr;
static void lupe(int x, int y) {
    int qx = constrain(x - LW / 2, 0, W - LW), qy = constrain(y - LH / 2, STATUS_H, H - LH);
    if (!lupeQuelle) lupeQuelle = (uint8_t*)ps_malloc(LW * LH);
    if (!lupeQuelle) return;
    for (int yy = 0; yy < LH; yy++) for (int xx = 0; xx < LW; xx++) lupeQuelle[yy * LW + xx] = display.readPixel(qx + xx, qy + yy) != WEISS;
    int kx = x < W / 2 ? W - LW * LF - 16 : 16, ky = STATUS_H + 10;
    display.fillRect(kx - 6, ky - 6, LW * LF + 12, LH * LF + 12, SCHWARZ);
    display.fillRect(kx - 3, ky - 3, LW * LF + 6, LH * LF + 6, WEISS);
    for (int yy = 0; yy < LH; yy++) for (int xx = 0; xx < LW; xx++)
        if (lupeQuelle[yy * LW + xx]) display.fillRect(kx + xx * LF, ky + yy * LF, LF, LF, SCHWARZ);
    display.drawRect(qx - 1, qy - 1, LW + 2, LH + 2, SCHWARZ);   // Ausschnitt im Original markieren
    teil(0, STATUS_H, W, H - STATUS_H);
    lupeAn = true;
}

static void betreten() {
    statuszeile(seite == 0 ? "2 · Schrift: Sans" : "2 · Schrift: Serif, Mono");
    seiteZeichnen();
    lupeAn = false;
}
static void neu() { display.fillScreen(WEISS); betreten(); teil(0, 0, W, H); }
static void taste(Knopf) { seite = (seite + 1) % 2; neu(); }
static void tipp(int x, int y) {
    if (lupeAn) { neu(); return; }
    lupe(x, y);
}

}  // namespace schriften
