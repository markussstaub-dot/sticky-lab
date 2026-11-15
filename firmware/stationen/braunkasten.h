// Station 4: Braun-Baukasten – alle Bedienelemente in der Firmware gezeichnet, in Ruhe, gedrückt und aktiv.
// Seite 1 Knöpfe, Seite 2 Regler (▲/▼). Tippen bedient das Element und misst Zeichen- und Refresh-Zeit.
#pragma once
#include "../braun.h"

namespace braunkasten {

static void knopfPunkt(int x, int y, bool);   // weiter unten: Drehknopf mit dem Finger drehen
static void knopfZeichnen();

static int seite = 0;
static uint32_t zeichenUs = 0, refreshMs = 0;
static const char* letztesElement = "";

// ---- Messzeile ----------------------------------------------------------------------------------------
static const int MY = 446;
static void messzeile() {
    display.fillRect(0, MY - 14, W, 30, WEISS);
    char b[96];
    if (*letztesElement) snprintf(b, sizeof b, T("%s: zeichnen %lu µs  ·  Teil-Refresh %lu ms   (▲▼ Seite %d/2)", "%s: draw %lu µs  ·  partial refresh %lu ms   (▲▼ page %d/2)"),
                                  letztesElement, (unsigned long)zeichenUs, (unsigned long)refreshMs, seite + 1);
    else snprintf(b, sizeof b, T("Tippen bedient ein Element  ·  ▲▼ Seite %d/2", "Tap operates an element  ·  ▲▼ page %d/2"), seite + 1);
    text(b, W / 2, MY, &FreeSans9pt7b, MC_DATUM);
}
/** Element neu zeichnen (zeichne), Zeit messen, Teil-Refresh des Bereichs, dann Messzeile */
template <typename F> static void bedienen(const char* name, int x, int y, int w, int h, F zeichne) {
    uint32_t t0 = micros();
    zeichne();
    zeichenUs = micros() - t0;
    refreshMs = teil(x, y, w, h);
    letztesElement = name;
    messzeile();
    teil(0, MY - 14, W, 30);
    char csv[24]; snprintf(csv, sizeof csv, "%s", name);
    messung("braun", csv, zeichenUs, "us", "zeichnen");
}

// ---- Seite 1: Knöpfe ------------------------------------------------------------------------------------
static const int SX[3] = { 200, 430, 660 };          // Spalten: Ruhe, gedrückt, aktiv
static const int ZY[4] = { 130, 218, 290, 372 };      // Zeilen: Doppelring, Pille, Werkzeug, Zahlentaste
static const int PW = 160, PH = 58;

static void knopfZelle(int zeile, int spalte, bool live = false, bool gedrueckt = false) {
    int cx = SX[spalte], cy = ZY[zeile];
    bool gd = live ? gedrueckt : spalte == 1;
    bool ak = !live && spalte == 2;
    switch (zeile) {
        case 0: display.fillRect(cx - 50, cy - 50, 100, 100, WEISS);
                if (ak) { display.fillCircle(cx, cy, 44, WEISS);
                          for (int i = 0; i < 3; i++) display.drawCircle(cx, cy, 44 - i, SCHWARZ);
                          display.fillCircle(cx, cy, 34, SCHWARZ); text(T("Stopp", "Stop"), cx, cy, &FreeSansBold9pt7b, MC_DATUM, WEISS); }
                else doppelring(cx, cy, 44, "Start", gd);
                break;
        case 1: pille(cx - PW / 2, cy - PH / 2, PW, PH, "± 10", ak ? R_AUSWAHL : R_NEBEN, gd); break;
        case 2: pille(cx - PW / 2, cy - PH / 2, PW, PH, T("Lupe", "Zoom"), ak ? R_AUSWAHL : R_WERKZEUG, gd); break;
        case 3: display.fillRect(cx - 40, cy - 40, 80, 80, WEISS);
                zahlTaste(cx, cy, 34, "7", gd);
                if (ak) for (int i = 0; i < 3; i++) display.drawCircle(cx, cy, 38 + i, SCHWARZ);
                break;
    }
}
static void zelleBox(int zeile, int spalte, int& x, int& y, int& w, int& h) {
    int cx = SX[spalte], cy = ZY[zeile];
    int hw = zeile == 0 ? 50 : zeile == 3 ? 42 : PW / 2 + 2, hh = zeile == 0 ? 50 : zeile == 3 ? 42 : PH / 2 + 2;
    x = cx - hw; y = cy - hh; w = 2 * hw; h = 2 * hh;
}
static void seiteKnoepfe() {
    text(T("Ruhe", "idle"), SX[0], 66, &FreeSans9pt7b, MC_DATUM);
    text(T("gedrückt", "pressed"), SX[1], 66, &FreeSans9pt7b, MC_DATUM);
    text(T("aktiv", "active"), SX[2], 66, &FreeSans9pt7b, MC_DATUM);
    static const char* rolle_de[4] = { "Haupt", "Neben", "Werkzeug", "Zahl" };
    static const char* rolle_en[4] = { "Main", "Minor", "Tool", "Digit" };
    const char* const* rolle = EN ? rolle_en : rolle_de;
    for (int z = 0; z < 4; z++) {
        text(rolle[z], 14, ZY[z], &FreeSans9pt7b, ML_DATUM);
        for (int s = 0; s < 3; s++) knopfZelle(z, s);
    }
}
static void tippKnoepfe(int x, int y) {
    for (int z = 0; z < 4; z++) {
        int bx, by, bw, bh;
        zelleBox(z, 0, bx, by, bw, bh);
        if (!drin(x, y, bx, by, bw, bh)) continue;
        static const char* name[4] = { "doppelring", "pille", "werkzeug", "zahltaste" };
        bedienen(name[z], bx, by, bw, bh, [&] { knopfZelle(z, 0, true, true); });
        delay(200);
        knopfZelle(z, 0, true, false);
        teil(bx, by, bw, bh);
        return;
    }
}

// ---- Seite 2: Regler --------------------------------------------------------------------------------------
static int schieberStufe = 2, knopfStufe = 5; static bool schalterAn = false; static int reiterNr = 0;
static const char* REITER_DE[3] = { "Tag", "Woche", "Monat" };
static const char* REITER_EN[3] = { "Day", "Week", "Month" };
#define REITER (EN ? REITER_EN : REITER_DE)
static const int SBX = 60, SBY = 120, SBW = 420, SBN = 5;
static const int DKX = 684, DKY = 168, DKR = 58, DKN = 11;   // 11 Stufen, mit dem Finger drehen
static const int SWX = 40, SWY = 225, SWW = 130, SWH = 58;
static const int RTX = 220, RTY = 225, RTW = 380, RTH = 58;
static const int TY = 360, TR = 32;
static const char* TASTEN[6] = { "7", "8", "9", "+", "-", "=" };

static void tastenReihe(int gedrueckt) {
    for (int i = 0; i < 6; i++) {
        int cx = 80 + i * 110;
        display.fillRect(cx - TR - 4, TY - TR - 4, 2 * TR + 8, 2 * TR + 8, WEISS);
        if (i >= 3) {   // Rechenzeichen als Nebensache: dünner Ring
            display.fillCircle(cx, TY, TR, i == gedrueckt ? SCHWARZ : WEISS);
            for (int k = 0; k < 2; k++) display.drawCircle(cx, TY, TR - k, SCHWARZ);
            text(TASTEN[i], cx, TY, &FreeSansBold12pt7b, MC_DATUM, i == gedrueckt ? WEISS : SCHWARZ);
        } else zahlTaste(cx, TY, TR, TASTEN[i], i == gedrueckt);
    }
}
static void seiteRegler() {
    text(T("Schieber, 5 Stufen", "Slider, 5 steps"), SBX, 70, &FreeSans9pt7b);
    schieber(SBX, SBY, SBW, SBN, schieberStufe);
    text(T("Drehknopf", "Rotary knob"), DKX + DKR + 20, 66, &FreeSans9pt7b, MR_DATUM);
    knopfZeichnen();
    schalter(SWX, SWY, SWW, SWH, schalterAn);
    reiter(RTX, RTY, RTW, RTH, REITER, 3, reiterNr);
    text(T("Tasten wie ET66: Zahlen schwarz, Rechenzeichen Ring", "Keys like ET66: digits black, operators ring"), 40, 304, &FreeSans9pt7b);
    tastenReihe(-1);
}
static void tippRegler(int x, int y) {
    if (drin(x, y, SBX - 24, SBY - 30, SBW + 48, 60)) {
        schieberStufe = schieberWert(SBX, SBW, SBN, x);
        bedienen("schieber", SBX - 24, SBY - 30, SBW + 48, 60, [] { schieber(SBX, SBY, SBW, SBN, schieberStufe); });
    } else if (drin(x, y, DKX - DKR - 22, DKY - DKR - 22, 2 * DKR + 44, 2 * DKR + 44)) {
        // Drehen übernimmt knopfPunkt() (jeder Touch-Punkt); ein Tipp setzt den Zeiger direkt auf die Stelle
        knopfPunkt(x, y, true);
    } else if (drin(x, y, SWX, SWY, SWW, SWH)) {
        schalterAn = !schalterAn;
        bedienen("schalter", SWX, SWY, SWW, SWH, [] { schalter(SWX, SWY, SWW, SWH, schalterAn); });
    } else if (drin(x, y, RTX, RTY, RTW, RTH)) {
        reiterNr = constrain((x - RTX) * 3 / RTW, 0, 2);
        bedienen("reiter", RTX, RTY, RTW, RTH, [] { reiter(RTX, RTY, RTW, RTH, REITER, 3, reiterNr); });
    } else {
        for (int i = 0; i < 6; i++) {
            int cx = 80 + i * 110;
            int dx = x - cx, dy = y - TY;
            if (dx * dx + dy * dy > (TR + 6) * (TR + 6)) continue;
            bedienen("taste", 40, TY - TR - 4, 680, 2 * TR + 8, [i] { tastenReihe(i); });
            delay(150);
            tastenReihe(-1);
            teil(40, TY - TR - 4, 680, 2 * TR + 8);
            return;
        }
    }
}

// ---- Drehknopf drehen: Winkel des Fingers um die Mitte → Stufe; gesammelt auffrischen wie beim Kritzeln ------
static bool knopfDreckig = false; static uint32_t knopfZeit = 0, knopfSeit = 0;
static void knopfZeichnen() {
    drehknopf(DKX, DKY, DKR, DKN, knopfStufe);
    char b[8]; snprintf(b, sizeof b, "%d", knopfStufe);
    display.fillCircle(DKX, DKY, 20, WEISS);
    text(b, DKX, DKY, &FreeSansBold12pt7b, MC_DATUM);
}
static void knopfPunkt(int x, int y, bool) {
    if (seite != 1) return;
    int dx = x - DKX, dy = y - DKY;
    if (dx * dx + dy * dy > (DKR + 30) * (DKR + 30) || dx * dx + dy * dy < 15 * 15) return;
    float a = atan2f(dy, dx) * RAD_TO_DEG;          // −180 … 180, 0 = rechts, im Uhrzeigersinn positiv
    float w = a - 135; while (w < 0) w += 360;       // Skala beginnt unten links (135°) und läuft 270° im Uhrzeigersinn
    if (w > 270) w = (w > 315) ? 0 : 270;            // Lücke unten: an das nähere Ende klemmen
    int st = lroundf(w / 270 * (DKN - 1));
    if (st != knopfStufe) {
        knopfStufe = st;
        knopfZeichnen();
        if (!knopfDreckig) knopfSeit = millis();
        knopfDreckig = true;
    }
    knopfZeit = millis();
}
static void tick() {
    tpPunkt = knopfPunkt;
    if (knopfDreckig && (millis() - knopfZeit > 120 || millis() - knopfSeit > 900)) {
        knopfDreckig = false;
        refreshMs = teil(DKX - DKR - 22, DKY - DKR - 22, 2 * DKR + 44, 2 * DKR + 44, true);
        letztesElement = "drehknopf"; zeichenUs = 0;
        messzeile(); teil(0, MY - 14, W, 30);
    }
}

// ---- Rahmen ---------------------------------------------------------------------------------------------
static void betreten() {
    statuszeile(seite == 0 ? T("4 · Braun: Knöpfe", "4 · Braun: buttons") : T("4 · Braun: Regler", "4 · Braun: controls"));
    if (seite == 0) seiteKnoepfe(); else seiteRegler();
    messzeile();
}
static void taste(Knopf k) {
    seite = (seite + 1) % 2;   // ▲ und ▼ blättern zwischen den zwei Seiten
    letztesElement = "";
    display.fillScreen(WEISS);
    betreten();
    teil(0, 0, W, H);
}
static void tipp(int x, int y) { if (seite == 0) tippKnoepfe(x, y); else tippRegler(x, y); }

}  // namespace braunkasten
