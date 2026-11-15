// Braun-Baukasten: Bedienelemente direkt in der Firmware gezeichnet (Regeln aus sticky-app/src/braun.js, EDC)
// Vorbild Braun ET66/BNE001: runde Tasten und Pillen, Rolle statt Farbe, ruhige Schrift, dicke Linien.
// Grundformen pille(), doppelring(), ringRR(), raster() stehen in anzeige.h; hier die zusammengesetzten Elemente.
#pragma once

/** Runde Zahlentaste wie beim ET66: schwarz mit weißer Ziffer; gedrückt weiß mit dickem Ring */
static void zahlTaste(int cx, int cy, int r, const char* s, bool gedrueckt = false) {
    display.fillCircle(cx, cy, r, gedrueckt ? WEISS : SCHWARZ);
    if (gedrueckt) for (int i = 0; i < 4; i++) display.drawCircle(cx, cy, r - i, SCHWARZ);
    text(s, cx, cy, &FreeSansBold12pt7b, MC_DATUM, gedrueckt ? SCHWARZ : WEISS);
}

/** Schieber mit Rastung: Bahn, Striche je Stufe, runder Knopf */
static void schieber(int x, int y, int w, int stufen, int wert) {
    display.fillRect(x - 24, y - 30, w + 48, 60, WEISS);
    display.fillRect(x, y - 1, w, 3, SCHWARZ);
    for (int i = 0; i < stufen; i++) {
        int sx = x + i * w / (stufen - 1);
        int l = (i == 0 || i == stufen - 1) ? 14 : 9;
        display.fillRect(sx - 1, y - l - 6, 3, l, SCHWARZ);
    }
    int kx = x + wert * w / (stufen - 1);
    display.fillCircle(kx, y, 20, SCHWARZ);
    display.fillCircle(kx, y, 6, WEISS);
}
static int schieberWert(int x, int w, int stufen, int tx) {
    int v = lround((tx - x) * (float)(stufen - 1) / w);
    return constrain(v, 0, stufen - 1);
}

/** Drehknopf: Skala über 270°, Knopf mit Zeiger; Wert 0..stufen-1 */
static void drehknopf(int cx, int cy, int r, int stufen, int wert) {
    display.fillRect(cx - r - 22, cy - r - 22, 2 * r + 44, 2 * r + 44, WEISS);
    const float a0 = 135 * DEG_TO_RAD, a1 = 405 * DEG_TO_RAD;   // von unten links im Uhrzeigersinn bis unten rechts
    for (int i = 0; i < stufen; i++) {
        float a = a0 + (a1 - a0) * i / (stufen - 1);
        int l = (i == wert) ? 14 : 7;
        display.drawWideLine(cx + cosf(a) * (r + 6), cy + sinf(a) * (r + 6), cx + cosf(a) * (r + 6 + l), cy + sinf(a) * (r + 6 + l),
                             i == wert ? 4 : 2, SCHWARZ);
    }
    for (int i = 0; i < 3; i++) display.drawCircle(cx, cy, r - i, SCHWARZ);
    float a = a0 + (a1 - a0) * wert / (stufen - 1);
    display.drawWideLine(cx + cosf(a) * r * 0.25f, cy + sinf(a) * r * 0.25f, cx + cosf(a) * (r - 8), cy + sinf(a) * (r - 8), 6, SCHWARZ);
}

/** Kippschalter als Pille: an = schwarze Bahn, weißer Knopf rechts; aus = Ring, schwarzer Knopf links */
static void schalter(int x, int y, int w, int h, bool an) {
    display.fillRoundRect(x, y, w, h, h / 2, WEISS);
    if (an) display.fillRoundRect(x, y, w, h, h / 2, SCHWARZ); else ringRR(x, y, w, h, h / 2, 3);
    int kr = h / 2 - 7;
    int kx = an ? x + w - h / 2 : x + h / 2;
    display.fillCircle(kx, y + h / 2, kr, an ? WEISS : SCHWARZ);
}

/** Reiter (Segmente in einer Pille), aktives Segment schwarz */
static void reiter(int x, int y, int w, int h, const char* const* namen, int n, int aktiv) {
    display.fillRoundRect(x, y, w, h, h / 2, WEISS);
    ringRR(x, y, w, h, h / 2, 3);
    int sw = w / n, e = 6;   // aktives Segment als eingesetzte schwarze Pille
    display.fillRoundRect(x + aktiv * sw + e, y + e, sw - 2 * e, h - 2 * e, (h - 2 * e) / 2, SCHWARZ);
    for (int i = 0; i < n; i++) text(namen[i], x + i * sw + sw / 2, y + h / 2, &FreeSans12pt7b, MC_DATUM, i == aktiv ? WEISS : SCHWARZ);
}
