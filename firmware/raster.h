// Raster und Dithering: Grauwerte (0 = schwarz … 255 = weiß) auf 1 bpp bringen
// Verfahren: Schwelle, Bayer 4×4 und 8×8 (geordnet), Floyd-Steinberg und Atkinson (Fehlerverteilung),
// Halbton (gruppierte Punkte wie im Zeitungsdruck). Quelle der Grauwerte: eine Funktion grau(x, y).
#pragma once

enum Verfahren : uint8_t { V_SCHWELLE, V_BAYER4, V_BAYER8, V_FLOYD, V_ATKINSON, V_HALBTON, V_ANZAHL };
static const char* V_NAMEN[V_ANZAHL] = { "Schwelle", "Bayer 4x4", "Bayer 8x8", "Floyd-Steinberg", "Atkinson", "Halbton" };   // auch CSV
static const char* V_NAMEN_EN[V_ANZAHL] = { "Threshold", "Bayer 4x4", "Bayer 8x8", "Floyd-Steinberg", "Atkinson", "Halftone" };
#define V_ANZEIGE(i) (EN ? V_NAMEN_EN[i] : V_NAMEN[i])

static const uint8_t BAYER8[8][8] = {
    { 0, 32, 8, 40, 2, 34, 10, 42 }, { 48, 16, 56, 24, 50, 18, 58, 26 }, { 12, 44, 4, 36, 14, 46, 6, 38 },
    { 60, 28, 52, 20, 62, 30, 54, 22 }, { 3, 35, 11, 43, 1, 33, 9, 41 }, { 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47, 7, 39, 13, 45, 5, 37 }, { 63, 31, 55, 23, 61, 29, 53, 21 } };
static const uint8_t BAYER4[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static const uint8_t HALBTON[4][4] = { { 12, 5, 6, 13 }, { 4, 0, 1, 7 }, { 11, 3, 2, 8 }, { 15, 10, 9, 14 } };   // wächst aus der Mitte

/** Zeichnet ein Rechteck aus Grauwerten mit dem Verfahren v. Gibt die Rechenzeit in µs zurück. */
template <typename G> static uint32_t dithern(int x0, int y0, int w, int h, Verfahren v, G grau) {
    uint32_t t0 = micros();
    if (v == V_FLOYD || v == V_ATKINSON) {
        // Fehler-Zeilen: drei Zeilen reichen für Atkinson (zwei nach unten)
        int16_t* z = (int16_t*)calloc(3 * (w + 4), sizeof(int16_t));
        if (!z) return 0;
        int16_t* zeile[3] = { z, z + (w + 4), z + 2 * (w + 4) };
        for (int y = 0; y < h; y++) {
            int16_t* e0 = zeile[y % 3] + 2; int16_t* e1 = zeile[(y + 1) % 3] + 2; int16_t* e2 = zeile[(y + 2) % 3] + 2;
            memset(zeile[(y + 2) % 3], 0, (w + 4) * sizeof(int16_t));
            for (int x = 0; x < w; x++) {
                int alt = grau(x, y) + e0[x];
                int neu = alt < 128 ? 0 : 255;
                int f = alt - neu;
                display.drawPixel(x0 + x, y0 + y, neu ? WEISS : SCHWARZ);
                if (v == V_FLOYD) {
                    e0[x + 1] += f * 7 / 16; e1[x - 1] += f * 3 / 16; e1[x] += f * 5 / 16; e1[x + 1] += f / 16;
                } else {   // Atkinson: 6/8 des Fehlers weiter, wirkt kontrastreicher
                    int a = f / 8;
                    e0[x + 1] += a; e0[x + 2] += a; e1[x - 1] += a; e1[x] += a; e1[x + 1] += a; e2[x] += a;
                }
            }
            memset(zeile[y % 3], 0, (w + 4) * sizeof(int16_t));
        }
        free(z);
    } else {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int g = grau(x, y), s;
                switch (v) {
                    case V_BAYER4:  s = BAYER4[y & 3][x & 3] * 16 + 8; break;
                    case V_BAYER8:  s = BAYER8[y & 7][x & 7] * 4 + 2; break;
                    case V_HALBTON: s = HALBTON[y & 3][x & 3] * 16 + 8; break;
                    default:        s = 128;
                }
                display.drawPixel(x0 + x, y0 + y, g < s ? SCHWARZ : WEISS);
            }
    }
    return micros() - t0;
}
