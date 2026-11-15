// Station 10: Tastatur – tippen auf e-Ink, Deutsch (QWERTZ mit Ä Ö Ü ß) und Englisch (QWERTY)
// Idee: die Eingabe ist schnell (Touch-Sammler alle 15 ms, Klick sofort), nur das Bild ist langsam (~0,85 s).
// Darum werden Tasten nie neu gezeichnet; nur die Textzeile wird gesammelt aufgefrischt, sobald ~0,3 s Pause ist.
// Übung: Zielsatz abtippen, gemessen werden Zeichen pro Minute und Fehler (Editierabstand zum Zielsatz).
#pragma once

namespace tastatur {

enum Sonder : uint8_t { T_ZEICHEN, T_SHIFT, T_LOESCHEN, T_SPRACHE, T_ENTER, T_LEER };
struct Taste { const char* klein; const char* gross; Sonder art; uint8_t breite; };   // breite in Zehntel einer Normaltaste

// Reihen je Sprache; Breiten so, dass jede Reihe 800 px füllt
static const Taste DE0[] = { {"q","Q"},{"w","W"},{"e","E"},{"r","R"},{"t","T"},{"z","Z"},{"u","U"},{"i","I"},{"o","O"},{"p","P"},{"ü","Ü"} };
static const Taste DE1[] = { {"a","A"},{"s","S"},{"d","D"},{"f","F"},{"g","G"},{"h","H"},{"j","J"},{"k","K"},{"l","L"},{"ö","Ö"},{"ä","Ä"} };
static const Taste DE2[] = { {"⇧","⇧",T_SHIFT,15},{"y","Y"},{"x","X"},{"c","C"},{"v","V"},{"b","B"},{"n","N"},{"m","M"},{"ß","ß"},{"<","<",T_LOESCHEN,15} };
static const Taste EN0[] = { {"q","Q"},{"w","W"},{"e","E"},{"r","R"},{"t","T"},{"y","Y"},{"u","U"},{"i","I"},{"o","O"},{"p","P"} };
static const Taste EN1[] = { {"a","A"},{"s","S"},{"d","D"},{"f","F"},{"g","G"},{"h","H"},{"j","J"},{"k","K"},{"l","L"} };
static const Taste EN2[] = { {"⇧","⇧",T_SHIFT,15},{"z","Z"},{"x","X"},{"c","C"},{"v","V"},{"b","B"},{"n","N"},{"m","M"},{"<","<",T_LOESCHEN,15} };
static const Taste R3[]  = { {"DE","EN",T_SPRACHE,15},{",",",",T_ZEICHEN,10},{"","",T_LEER,50},{".",".",T_ZEICHEN,10},{"OK","OK",T_ENTER,15} };

struct Reihe { const Taste* t; uint8_t n; };
static bool englisch = false, shift = false;
static Reihe reihe(int r) {
    if (r == 3) return { R3, 5 };
    if (englisch) return r == 0 ? Reihe{ EN0, 10 } : r == 1 ? Reihe{ EN1, 9 } : Reihe{ EN2, 9 };
    return r == 0 ? Reihe{ DE0, 11 } : r == 1 ? Reihe{ DE1, 11 } : Reihe{ DE2, 10 };
}
static int breiteSumme(const Reihe& r) { int s = 0; for (int i = 0; i < r.n; i++) s += r.t[i].breite ? r.t[i].breite : 10; return s; }

static const int KY = 168, KH = 78, TY = STATUS_H, TH = KY - STATUS_H - 4;   // Tastatur / Textbereich
static const int RAND = 4;

/** Lage von Taste i in Reihe r */
static void lage(int r, int i, int& x, int& y, int& w, int& h) {
    Reihe rr = reihe(r);
    int summe = breiteSumme(rr), pos = 0;
    for (int k = 0; k < i; k++) pos += rr.t[k].breite ? rr.t[k].breite : 10;
    int b = rr.t[i].breite ? rr.t[i].breite : 10;
    x = pos * W / summe; w = (pos + b) * W / summe - x; y = KY + r * KH; h = KH;
}

static void tasteZeichnen(int r, int i) {
    int x, y, w, h; lage(r, i, x, y, w, h);
    const Taste& t = reihe(r).t[i];
    display.fillRect(x, y, w, h, WEISS);
    int bx = x + RAND, by = y + RAND, bw = w - 2 * RAND, bh = h - 2 * RAND;
    bool aktiv = (t.art == T_SHIFT && shift);
    if (t.art == T_ZEICHEN || t.art == T_LEER) ringRR(bx, by, bw, bh, 12, 2);
    else if (aktiv) display.fillRoundRect(bx, by, bw, bh, 12, SCHWARZ);
    else { display.fillRoundRect(bx, by, bw, bh, 12, SCHWARZ); display.fillRoundRect(bx + 4, by + 4, bw - 8, bh - 8, 9, WEISS); }   // Werkzeug: dicker Rand
    const char* l = t.art == T_SPRACHE ? (englisch ? "EN" : "DE") : (shift ? t.gross : t.klein);
    if (t.art == T_LOESCHEN) {   // Pfeil nach links
        int cx = bx + bw / 2, cy = by + bh / 2;
        display.fillTriangle(cx - 16, cy, cx - 4, cy - 10, cx - 4, cy + 10, SCHWARZ);
        display.fillRect(cx - 4, cy - 3, 20, 6, SCHWARZ);
    } else if (t.art == T_SHIFT) {   // Pfeil nach oben
        int cx = bx + bw / 2, cy = by + bh / 2; uint32_t f = aktiv ? WEISS : SCHWARZ;
        display.fillTriangle(cx - 12, cy, cx + 12, cy, cx, cy - 14, f);
        display.fillRect(cx - 5, cy, 10, 12, f);
    } else if (t.art == T_LEER) {
        text(englisch ? "space" : "Leerzeichen", bx + bw / 2, by + bh / 2, &FreeSans9pt7b, MC_DATUM);
    } else text(l, bx + bw / 2, by + bh / 2, t.art == T_ZEICHEN ? &FreeSans18pt7b : &FreeSansBold12pt7b, MC_DATUM, aktiv ? WEISS : SCHWARZ);
}
static void tastaturZeichnen() {
    display.fillRect(0, KY, W, H - KY, WEISS);
    for (int r = 0; r < 4; r++) for (int i = 0; i < reihe(r).n; i++) tasteZeichnen(r, i);
}

// ---- Text und Übung ------------------------------------------------------------------------------------
static char textPuffer[160] = "";
static bool dreckig = false; static uint32_t dreckigSeit = 0;
static uint32_t letzterAnschlag = 0, ersterAnschlag = 0;
static uint32_t anschlaege = 0, refreshs = 0, refreshMs = 0;
static const char* ZIEL_DE = "Franz jagt im komplett verwahrlosten Taxi quer durch Bayern.";
static const char* ZIEL_EN = "The quick brown fox jumps over the lazy dog.";
static char ergebnis[80] = "";

static int zeichenZahl(const char* s) { int n = 0; for (; *s; s++) if ((*s & 0xC0) != 0x80) n++; return n; }
/** Editierabstand nach Zeichen (UTF-8-sicher: Codepoints vergleichen) */
static int abstand(const char* a, const char* b) {
    static uint32_t ca[160], cb[160];
    auto lese = [](const char* s, uint32_t* o) { int n = 0; while (*s && n < 159) { uint32_t c = (uint8_t)*s++; if (c >= 0xC0) { while ((*s & 0xC0) == 0x80) c = c << 8 | (uint8_t)*s++; } o[n++] = c; } return n; };
    int n = lese(a, ca), m = lese(b, cb);
    static uint16_t d[2][161];
    for (int j = 0; j <= m; j++) d[0][j] = j;
    for (int i = 1; i <= n; i++) {
        d[i & 1][0] = i;
        for (int j = 1; j <= m; j++) {
            int k = d[(i - 1) & 1][j - 1] + (ca[i - 1] != cb[j - 1]);
            k = min(k, d[(i - 1) & 1][j] + 1); k = min(k, d[i & 1][j - 1] + 1);
            d[i & 1][j] = k;
        }
    }
    return d[n & 1][m];
}

static void textZeichnen() {
    display.fillRect(0, TY, W, TH, WEISS);
    text(englisch ? "Practice:" : "Übung:", 12, TY + 10, &FreeSans9pt7b);
    text(englisch ? ZIEL_EN : ZIEL_DE, englisch ? 92 : 80, TY + 10, &FreeSans9pt7b);
    // Eingabe: die letzten Zeichen, die in eine Zeile passen
    const char* zeig = textPuffer;
    while (zeichenZahl(zeig) > 42) { zeig++; while ((*zeig & 0xC0) == 0x80) zeig++; }
    char z[180]; snprintf(z, sizeof z, "%s_", zeig);
    text(z, 12, TY + 58, &FreeSans18pt7b, ML_DATUM);
    text(*ergebnis ? ergebnis : (englisch ? "OK = evaluate  ·  language bottom left" : "OK = auswerten  ·  Sprache unten links"), 12, TY + TH - 14, &FreeSans9pt7b, ML_DATUM);
    display.drawFastHLine(0, KY - 3, W, SCHWARZ);
}

static void auswerten() {
    const char* ziel = englisch ? ZIEL_EN : ZIEL_DE;
    int n = zeichenZahl(textPuffer);
    if (n < 3) return;
    uint32_t dauer = letzterAnschlag - ersterAnschlag; if (dauer < 1) dauer = 1;
    float min_ = dauer / 60000.0f;
    int zpm = lroundf(n / min_), fehler = abstand(textPuffer, ziel);
    snprintf(ergebnis, sizeof ergebnis, englisch ? "%d chars/min, %d errors, text refresh %lu ms on average" : "%d Zeichen/min, %d Fehler, Text-Refresh %lu ms im Mittel",
             zpm, fehler, (unsigned long)(refreshs ? refreshMs / refreshs : 0));
    char notiz[40]; snprintf(notiz, sizeof notiz, "%s %d fehler", englisch ? "en" : "de", fehler);
    messung("tastatur", "zeichen-pro-min", zpm, "zpm", notiz);
    textPuffer[0] = 0; anschlaege = 0; refreshs = 0; refreshMs = 0; ersterAnschlag = 0;
    dreckig = true;
}

static void anschlag(const Taste& t) {
    uint32_t jetzt = millis();
    if (!ersterAnschlag) ersterAnschlag = jetzt;
    letzterAnschlag = jetzt; anschlaege++;
    size_t l = strlen(textPuffer);
    switch (t.art) {
        case T_ZEICHEN: case T_LEER: {
            const char* s = t.art == T_LEER ? " " : (shift ? t.gross : t.klein);
            if (l + strlen(s) < sizeof textPuffer - 1) strcat(textPuffer, s);
            if (shift) { shift = false; tastaturZeichnen(); teil(0, KY, W, H - KY); }   // Großbuchstaben: Tasten zurück
            *ergebnis = 0;
            break;
        }
        case T_LOESCHEN:
            while (l > 0 && (textPuffer[l - 1] & 0xC0) == 0x80) l--;   // UTF-8-Folgebytes
            if (l > 0) l--;
            textPuffer[l] = 0;
            break;
        case T_SHIFT: shift = !shift; tastaturZeichnen(); teil(0, KY, W, H - KY); return;
        case T_SPRACHE: englisch = !englisch; shift = false; textPuffer[0] = 0; *ergebnis = 0;
                        tastaturZeichnen(); textZeichnen(); teil(0, TY, W, H - TY); return;
        case T_ENTER: auswerten(); break;
    }
    if (!dreckig) dreckigSeit = millis();
    dreckig = true;
}

// ---- Rahmen ------------------------------------------------------------------------------------------------
static void betreten() {
    // jede Übung beginnt frisch
    textPuffer[0] = 0; *ergebnis = 0; anschlaege = refreshs = refreshMs = 0; ersterAnschlag = 0; dreckig = false; shift = false;
    statuszeile("10 · Tastatur");
    textZeichnen();
    tastaturZeichnen();
}
static void tick() {
    // gesammelt auffrischen: erst wenn ~0,3 s keine Taste kam (oder spätestens nach 1,5 s Dauertippen)
    if (dreckig && (millis() - letzterAnschlag > 300 || millis() - dreckigSeit > 1500)) {
        dreckig = false;
        textZeichnen();
        refreshMs += teil(0, TY, W, TH, true);
        refreshs++;
    }
}
static void tipp(int x, int y) {
    if (y < KY) return;
    int r = min(3, (y - KY) / KH);
    for (int i = 0; i < reihe(r).n; i++) {
        int tx, ty, tw, th; lage(r, i, tx, ty, tw, th);
        if (x >= tx && x < tx + tw) { anschlag(reihe(r).t[i]); return; }
    }
}
static void taste(Knopf k) { if (k == K_DOWN) { Taste t = { "<", "<", T_LOESCHEN, 15 }; anschlag(t); } }

}  // namespace tastatur
