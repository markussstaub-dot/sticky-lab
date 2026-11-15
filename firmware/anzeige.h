// Anzeige: Seeed_GFX2 (mit Sticky-Patch), Refresh-Arten mit Zeitmessung, Braun-Grundformen, Statuszeile
#pragma once
#include <Seeed_GFX.h>
#include <font/GFXFF/FreeSans9pt7b.h>
#include <font/GFXFF/FreeSans12pt7b.h>
#include <font/GFXFF/FreeSansBold9pt7b.h>
#include <font/GFXFF/FreeSansBold12pt7b.h>
#include <font/GFXFF/FreeSansBold24pt7b.h>

Seeed_GFX display(Seeed_Product::reTerminal_Sticky);
extern bool gfxPartialWachHalten;   // aus dem Bibliotheks-Patch (reterminal-sticky/firmware-ble/bibliothek-patch)

static const uint32_t SCHWARZ = TFT_BLACK, WEISS = TFT_WHITE;

// ---- Panel-Profil -------------------------------------------------------------------------------------
// Alles, was vom Panel abhängt, steht hier. Ein späterer Sticky mit anderem Panel (z. B. Farbe) bekommt ein
// eigenes Profil; Stationen und Console lesen Größe, Farben und Pixeldichte von hier statt sie fest anzunehmen.
struct Panel {
    const char* name;
    int w, h;          // Pixel quer
    int farben;        // 2 = Schwarz/Weiß; Farb-e-Ink z. B. 6 (Spectra 6) oder 7
    float pxProMm;     // Pixeldichte, für Originalgröße und Messungen in mm
    bool teilRefresh;  // kann das Panel Teil-Refresh? (Farb-e-Ink meist nicht)
};
static const Panel PANEL = { "Sticky 3.97in SW", 800, 480, 2, 9.25f, true };

/** Erkannter Display-Controller: der Sticky hat je Gerät SSD1677 oder SSD2677 (Seeed_GFX2 erkennt ihn bei begin()) */
static const char* panelChip() { return display.panel().driver().name(); }
static bool panelGrau4() { return display.panel().driver().supportsGrayRefresh(4); }

// ---- Refresh-Arten ---------------------------------------------------------------
enum Art : uint8_t { ART_TEIL = 0, ART_TEIL_WACH, ART_SCHNELL, ART_VOLL, ART_ANZAHL };
static const char* ART_NAME[ART_ANZAHL] = { "Teil", "Teil wach", "Schnell", "Voll" };
static uint32_t teilSeitVoll = 0;   // Teil- und Schnell-Refreshes seit dem letzten Voll-Refresh

/** Teil-Refresh eines Rechtecks; x und Breite werden auf ganze Bytes (8 px) erweitert. Gibt die Dauer in ms zurück. */
static uint32_t teil(int x, int y, int w, int h, bool wach = false) {
    int x0 = max(0, x & ~7), x1 = min(W, (x + w + 7) & ~7);
    y = max(0, y); h = min(H - y, h);
    gfxPartialWachHalten = wach;
    uint32_t t0 = millis();
    display.updatePartial(x0, y, x1 - x0, h);
    uint32_t ms = millis() - t0;
    gfxPartialWachHalten = false;
    teilSeitVoll++;
    return ms;
}
static uint32_t voll() {
    uint32_t t0 = millis();
    display.update();
    teilSeitVoll = 0;
    return millis() - t0;
}
static uint32_t schnell() {
    uint32_t t0 = millis();
    display.updateFast();
    teilSeitVoll++;
    return millis() - t0;
}

// ---- Text ---------------------------------------------------------------------------
// Die FreeSans-Schriften kennen nur ASCII. text() setzt Umlaute selbst (Grundbuchstabe + zwei Punkte),
// ß wird „ss“, · wird ein Mittelpunkt, ▲/▼ werden kleine Dreiecke. Datum wie TFT_eSPI: TL/ML/MC/MR/TC.
static void umwandeln(const char* s, char* a, size_t n, uint8_t* art) {
    // art[i]: 0 normal, 1 Umlaut-Punkte, 2 Mittelpunkt, 3 Dreieck hoch, 4 Dreieck runter
    size_t j = 0;
    for (const uint8_t* p = (const uint8_t*)s; *p && j + 2 < n; ) {
        uint8_t c = p[0];
        if (c == 0xC3 && p[1]) {
            char b = 0;
            switch (p[1]) { case 0xA4: b = 'a'; break; case 0xB6: b = 'o'; break; case 0xBC: b = 'u'; break;
                            case 0x84: b = 'A'; break; case 0x96: b = 'O'; break; case 0x9C: b = 'U'; break; }
            if (b) { art[j] = 1; a[j++] = b; p += 2; continue; }
            if (p[1] == 0x9F) { art[j] = 0; a[j++] = 's'; art[j] = 0; a[j++] = 's'; p += 2; continue; }
        }
        if (c == 0xC2 && p[1] == 0xB7) { art[j] = 2; a[j++] = ' '; p += 2; continue; }
        if (c == 0xC2 && p[1] == 0xB1) { art[j] = 0; a[j++] = '+'; art[j] = 0; a[j++] = '/'; art[j] = 0; a[j++] = '-'; p += 2; continue; }   // ±
        if (c == 0xC2 && p[1] == 0xB5) { art[j] = 0; a[j++] = 'u'; p += 2; continue; }   // µ
        if (c == 0xC2 && p[1] == 0xB0) { art[j] = 5; a[j++] = ' '; p += 2; continue; }   // ° als kleiner Ring
        if (c == 0xE2 && p[1] == 0x96 && (p[2] == 0xB2 || p[2] == 0xBC)) { art[j] = p[2] == 0xB2 ? 3 : 4; a[j++] = ' '; art[j] = 0; a[j++] = ' '; art[j] = 0; a[j++] = ' '; p += 3; continue; }
        if (c < 0x80) { art[j] = 0; a[j++] = c; p++; } else p++;   // Unbekanntes überspringen
    }
    a[j] = 0;
}
static const GFXglyph* glyphe(const GFXfont* f, char c) {
    if ((uint8_t)c < f->first || (uint8_t)c > f->last) return nullptr;
    return &f->glyph[(uint8_t)c - f->first];
}
/** Breite der ersten n Zeichen als Summe der Vorschübe (textWidth() lässt ein Leerzeichen am Ende weg) */
static int vorschub(const GFXfont* f, const char* a, int n = -1) {
    int w = 0;
    for (int i = 0; a[i] && (n < 0 || i < n); i++) { const GFXglyph* g = glyphe(f, a[i]); if (g) w += g->xAdvance; }
    return w;
}
static void text(const char* s, int x, int y, const GFXfont* f, uint8_t datum = TL_DATUM, uint32_t farbe = SCHWARZ) {
    char a[128]; uint8_t art[128];
    umwandeln(s, a, sizeof a, art);
    display.setFreeFont(f);
    display.setTextSize(1);
    display.setTextColor(farbe);
    int w = vorschub(f, a);
    const GFXglyph* gH = glyphe(f, 'H');
    int cap = gH ? -gH->yOffset : 10;
    int links = x, basis = y;
    switch (datum) {
        case TC_DATUM: links = x - w / 2; basis = y + cap; break;
        case TR_DATUM: links = x - w;     basis = y + cap; break;
        case ML_DATUM: basis = y + cap / 2; break;
        case MC_DATUM: links = x - w / 2; basis = y + cap / 2; break;
        case MR_DATUM: links = x - w;     basis = y + cap / 2; break;
        default:       basis = y + cap; break;   // TL
    }
    display.setTextDatum(L_BASELINE);
    display.drawString(a, links, basis);
    int r = max(1, (int)f->yAdvance / 22);
    for (int i = 0; a[i]; i++) {
        if (!art[i]) continue;
        int cx0 = links + vorschub(f, a, i);
        const GFXglyph* g = glyphe(f, a[i]);
        if (!g) continue;
        int mitte = cx0 + g->xOffset + g->width / 2;
        if (art[i] == 1) {
            // Punkte über dem Buchstaben, Hintergrund freiräumen (Großbuchstaben reichen bis an die Punkte)
            int oben = basis + g->yOffset - r * 2 - 1;
            int d = max(r + 1, (int)g->width / 4);
            display.fillCircle(mitte - d, oben, r, farbe);
            display.fillCircle(mitte + d, oben, r, farbe);
        } else if (art[i] == 5) {
            int rr = max(2, (int)f->yAdvance / 12);
            display.drawCircle(cx0 + rr + 1, basis - cap + rr, rr, farbe);
        } else if (art[i] == 2) {
            display.fillCircle(cx0 + g->xAdvance / 2, basis - cap / 2, max(2, (int)f->yAdvance / 14), farbe);
        } else {
            // Dreieck in den Platz von drei Leerzeichen (nichts zu übermalen)
            int platz = 3 * g->xAdvance;
            mitte = cx0 + platz / 2;
            int b = max(8, platz - 2), h = cap * 3 / 4, y0 = basis - h;
            if (art[i] == 3) display.fillTriangle(mitte - b / 2, basis, mitte + b / 2, basis, mitte, y0, farbe);
            else display.fillTriangle(mitte - b / 2, y0, mitte + b / 2, y0, mitte, basis, farbe);
        }
    }
}

// ---- Braun-Grundformen (Regeln aus sticky-app/src/braun.js) --------------------------
// Rolle statt Farbe: Hauptaktion = Doppelring, Zahlen/Auswahl = schwarz, Werkzeug = gerasterter Rand,
// Nebensache = dünner Ring. Knöpfe mindestens ~120 × 58 px.
enum Rolle : uint8_t { R_NEBEN, R_AUSWAHL, R_WERKZEUG, R_HAUPT };

/** Schachbrett 2×2 px (50 % Grau) in ein Rechteck */
static void raster(int x, int y, int w, int h, int s = 2) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            display.drawPixel(xx, yy, (((xx - x) / s + (yy - y) / s) & 1) ? SCHWARZ : WEISS);
}

/** Ring mit Dicke d als abgerundetes Rechteck */
static void ringRR(int x, int y, int w, int h, int r, int d, uint32_t farbe = SCHWARZ) {
    for (int i = 0; i < d; i++) display.drawRoundRect(x + i, y + i, w - 2 * i, h - 2 * i, max(0, r - i), farbe);
}

static void pille(int x, int y, int w, int h, const char* s, Rolle rolle, bool gedrueckt = false) {
    int r = h / 2;
    display.fillRoundRect(x, y, w, h, r, WEISS);
    bool voll = rolle == R_AUSWAHL || gedrueckt;
    if (voll) display.fillRoundRect(x, y, w, h, r, SCHWARZ);
    else if (rolle == R_WERKZEUG) {
        // Rasterrand: Raster in die Form, Innenfläche wieder weiß
        display.fillRoundRect(x, y, w, h, r, SCHWARZ);
        for (int yy = y; yy < y + h; yy++)
            for (int xx = x; xx < x + w; xx++)
                if (((xx + yy) & 1) && display.readPixel(xx, yy) == SCHWARZ) display.drawPixel(xx, yy, WEISS);
        display.fillRoundRect(x + 6, y + 6, w - 12, h - 12, r - 6, WEISS);
    } else ringRR(x, y, w, h, r, rolle == R_HAUPT ? 4 : 2);
    text(s, x + w / 2, y + h / 2, &FreeSans12pt7b, MC_DATUM, voll ? WEISS : SCHWARZ);
}

static void doppelring(int cx, int cy, int r, const char* s, bool gedrueckt = false) {
    display.fillCircle(cx, cy, r, gedrueckt ? SCHWARZ : WEISS);
    uint32_t v = gedrueckt ? WEISS : SCHWARZ;
    int innen = r < 52 ? 7 : 10;   // kleine Ringe: innerer Ring näher am äußeren, damit das Wort Platz hat
    for (int i = 0; i < 3; i++) { display.drawCircle(cx, cy, r - i, SCHWARZ); display.drawCircle(cx, cy, r - innen - i, v); }
    text(s, cx, cy, r < 52 ? &FreeSansBold9pt7b : &FreeSansBold12pt7b, MC_DATUM, v);
}

// ---- Statuszeile ------------------------------------------------------------------------
static const int STATUS_H = 46;
static int akkuProzent();
static void statuszeile(const char* name) {
    display.fillRect(0, 0, W, STATUS_H, WEISS);
    text(name, 20, 23, &FreeSansBold12pt7b, ML_DATUM);
    char r[40];
    int p = akkuProzent();
    if (p >= 0) snprintf(r, sizeof r, "%d %%  ·  LAB", p); else snprintf(r, sizeof r, "LAB");
    text(r, W - 20, 23, &FreeSans12pt7b, MR_DATUM);
    display.fillRect(0, STATUS_H - 2, W, 2, SCHWARZ);
}
