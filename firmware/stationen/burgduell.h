// Station 9: Burgduell – eigenes, rundenbasiertes Artillerie-Spiel, lokal auf dem Sticky (kein Handy nötig)
// Zielen: Tipp ins Feld (Richtung und Abstand von der eigenen Kanone = Winkel und Kraft), ▲▼ = Winkel fein.
// Feuer: Doppelring unten rechts. Flugbahn als Punktspur, Krater im Gelände, 3 Treffer zerstören eine Burg.
// Gegner: zweiter Mensch (reihum) oder der Sticky selbst (Pille unten links).
#pragma once

namespace burgduell {

static const int FY = STATUS_H, FH = 410 - STATUS_H;        // Spielfeld
static const int HUD_Y = 412;
static const int FEUER_X = 748, FEUER_Y = 445, FEUER_R = 36;
static const int GX = 8, GY = 420, GW = 150, GH = 52;       // Pille Gegner
// Regler unten: Winkel − / +, Pulver − / +  (runde Knöpfe, r = 22)
static const int RY = 452, RR = 22, WM_X = 200, WP_X = 320, PM_X = 400, PP_X = 520;
static const int BW = 46, BH = 40;                          // Burg

static int16_t boden[W];                                    // Geländehöhe (y) je Spalte
static int burgX[2], hp[2];
static int am = 0;                                          // wer ist am Zug
static int winkel[2] = { 45, 45 }, kraft[2] = { 55, 55 };
static int wind = 0;
static bool gegenSticky = false, vorbei = false;
static int zuege = 0;
static const int MAXSPUR = 400;
static int16_t spurX[MAXSPUR], spurY[MAXSPUR]; static int spurN = 0;
static char meldung[64] = "";

// ---- Welt -----------------------------------------------------------------------------------------------
static void neueWelt() {
    float a1 = random(10, 40), a2 = random(5, 25), p1 = random(0, 628) / 100.0f, p2 = random(0, 628) / 100.0f;
    float f1 = random(6, 14) / 1000.0f, f2 = random(15, 30) / 1000.0f;
    for (int x = 0; x < W; x++) boden[x] = 330 - a1 * sinf(x * f1 + p1) - a2 * sinf(x * f2 + p2) - 40 * sinf(x * PI / W);
    burgX[0] = 70; burgX[1] = W - 70;
    for (int b = 0; b < 2; b++) {   // Bauplatz ebnen
        int y = boden[burgX[b]];
        for (int x = burgX[b] - BW / 2 - 6; x <= burgX[b] + BW / 2 + 6; x++) boden[x] = y;
    }
    hp[0] = hp[1] = 3; am = 0; zuege = 0; vorbei = false; spurN = 0;
    wind = random(-5, 6);
    winkel[0] = winkel[1] = 45; kraft[0] = kraft[1] = 55;
    snprintf(meldung, sizeof meldung, "%s", T("Voll-Fahne beginnt", "Solid flag starts"));   // Bedienung steht in der Hilfe
}
static void kanone(int b, int& x, int& y) { x = burgX[b] + (b == 0 ? 10 : -10); y = boden[burgX[b]] - BH - 10; }

static void burgZeichnen(int b) {
    int x0 = burgX[b] - BW / 2, y0 = boden[burgX[b]] - BH;
    display.fillRect(x0, y0, BW, BH, SCHWARZ);
    // Zinnen: je verlorenem Lebenspunkt fehlt eine
    for (int i = 0; i < 3; i++) if (i < hp[b]) display.fillRect(x0 + 2 + i * 16, y0 - 8, 10, 8, SCHWARZ);
    display.fillRect(burgX[b] - 6, y0 + BH - 16, 12, 16, WEISS);   // Tor
    // Turm mit Fahne (Spieler 1 voll, Spieler 2 Ring)
    int tx = burgX[b] + (b == 0 ? 10 : -10);
    display.fillRect(tx - 5, y0 - 18, 10, 18, SCHWARZ);
    display.drawFastVLine(tx, y0 - 34, 16, SCHWARZ);
    int fx = b == 0 ? tx : tx - 14;
    if (b == 0) display.fillTriangle(fx, y0 - 34, fx + 14, y0 - 29, fx, y0 - 24, SCHWARZ);
    else display.drawTriangle(fx + 14, y0 - 34, fx, y0 - 29, fx + 14, y0 - 24, SCHWARZ);
    if (hp[b] == 0) { display.drawLine(x0 - 4, y0 - 10, x0 + BW + 4, y0 + BH, WEISS); display.drawLine(x0 - 4, y0 + BH, x0 + BW + 4, y0 - 10, WEISS); }
}

static void feldZeichnen(bool zielhilfe) {
    display.fillRect(0, FY, W, FH, WEISS);
    for (int x = 0; x < W; x++) {   // Gelände: Raster 50 %, Kante schwarz
        for (int y = boden[x] + 2; y < FY + FH; y++) display.drawPixel(x, y, ((x + y) & 1) ? SCHWARZ : WEISS);
        display.drawFastVLine(x, boden[x], 2, SCHWARZ);
    }
    burgZeichnen(0); burgZeichnen(1);
    for (int i = 0; i < spurN; i += 3) display.fillCircle(spurX[i], spurY[i], 2, SCHWARZ);   // letzte Flugbahn
    // Wind oben in der Mitte
    char b[24];
    if (wind == 0) snprintf(b, sizeof b, "%s", T("Wind still", "No wind"));
    else snprintf(b, sizeof b, "Wind %d", abs(wind));
    text(b, W / 2, FY + 16, &FreeSans9pt7b, MC_DATUM);
    if (*meldung) text(meldung, 10, FY + 10, &FreeSans9pt7b);   // Meldung oben links im Feld
    if (wind) {
        int l = abs(wind) * 8, x = W / 2 - l / 2, y = FY + 34;
        display.fillRect(x, y - 1, l, 3, SCHWARZ);
        if (wind > 0) display.fillTriangle(x + l, y - 6, x + l + 9, y, x + l, y + 6, SCHWARZ);
        else display.fillTriangle(x, y - 6, x - 9, y, x, y + 6, SCHWARZ);
    }
    if (zielhilfe && !vorbei) {   // gestrichelte Ziellinie, Länge ~ Kraft
        int kx, ky; kanone(am, kx, ky);
        float a = winkel[am] * DEG_TO_RAD; int dir = am == 0 ? 1 : -1;
        float l = kraft[am] * 2.6f;
        for (float t = 8; t < l; t += 10)
            display.fillCircle(kx + dir * cosf(a) * t, ky - sinf(a) * t, 1, SCHWARZ);
    }
}

static void rundKnopf(int cx, const char* s) {
    display.fillCircle(cx, RY, RR, WEISS);
    for (int i = 0; i < 2; i++) display.drawCircle(cx, RY, RR - i, SCHWARZ);
    text(s, cx, RY, &FreeSansBold12pt7b, MC_DATUM);
}
static void hud() {
    display.fillRect(0, HUD_Y, W, H - HUD_Y, WEISS);
    display.drawFastHLine(0, HUD_Y, W, SCHWARZ);
    pille(GX, GY, GW, GH, gegenSticky ? T("gegen Sticky", "vs. Sticky") : T("2 Spieler", "2 players"), R_NEBEN);
    if (vorbei) {
        text(meldung, 180, GY + 14, &FreeSansBold12pt7b, ML_DATUM);
        text(T("Tippen = neues Spiel", "Tap = new game"), 180, GY + 40, &FreeSans9pt7b, ML_DATUM);
        return;
    }
    char b[24];
    // Winkel
    text(T("Winkel", "Angle"), (WM_X + WP_X) / 2, HUD_Y + 10, &FreeSans9pt7b, MC_DATUM);
    rundKnopf(WM_X, "-"); rundKnopf(WP_X, "+");
    snprintf(b, sizeof b, "%d°", winkel[am]); text(b, (WM_X + WP_X) / 2, RY, &FreeSansBold12pt7b, MC_DATUM);
    // Pulver
    text(T("Pulver", "Powder"), (PM_X + PP_X) / 2, HUD_Y + 10, &FreeSans9pt7b, MC_DATUM);
    rundKnopf(PM_X, "-"); rundKnopf(PP_X, "+");
    snprintf(b, sizeof b, "%d", kraft[am]); text(b, (PM_X + PP_X) / 2, RY, &FreeSansBold12pt7b, MC_DATUM);
    // wer ist dran (klein über dem Feuer-Knopf)
    text(am == 0 ? T("Voll-Fahne", "Solid flag") : (gegenSticky ? "Sticky" : T("Ring-Fahne", "Ring flag")), 640, HUD_Y + 10, &FreeSans9pt7b, MC_DATUM);
    doppelring(FEUER_X, FEUER_Y, FEUER_R, T("Feuer", "Fire"));
}
static void statusZug() {
    char t[48];
    snprintf(t, sizeof t, T("9 · Burgduell  ·  %d:%d", "9 · Castle duel  ·  %d:%d"), hp[0], hp[1]);
    statuszeile(t);
}
static void allesNeu() { statusZug(); feldZeichnen(true); hud(); }

// ---- Schuss -----------------------------------------------------------------------------------------------
/** Simuliert einen Schuss; liefert Einschlag (x, y) und ob eine Burg getroffen wurde (-1 = keine) */
static int simuliere(int b, int w, int k, int& ex, int& ey, bool aufzeichnen) {
    int kx, ky; kanone(b, kx, ky);
    float a = w * DEG_TO_RAD, dir = b == 0 ? 1 : -1;
    float v = k * 0.105f, vx = dir * cosf(a) * v, vy = -sinf(a) * v, x = kx, y = ky;
    if (aufzeichnen) spurN = 0;
    for (int i = 0; i < 2000; i++) {
        vx += wind * 0.004f; vy += 0.12f;
        x += vx; y += vy;
        if (aufzeichnen && spurN < MAXSPUR && (i & 1)) { spurX[spurN] = x; spurY[spurN] = y; spurN++; }
        if (x < 0 || x >= W || y > FY + FH) { ex = x; ey = y; return -2; }
        for (int bb = 0; bb < 2; bb++) {
            int x0 = burgX[bb] - BW / 2, y0 = boden[burgX[bb]] - BH - 34;
            if (i > 6 && x >= x0 && x < x0 + BW && y >= y0 && y < boden[burgX[bb]]) { ex = x; ey = y; return bb; }
        }
        if (y >= FY && y >= boden[(int)x]) { ex = x; ey = y; return -1; }
    }
    ex = x; ey = y; return -2;
}
static void krater(int cx, int cy, int r) {
    for (int x = max(0, cx - r); x < min(W, cx + r); x++) {
        int dy = sqrtf(r * r - (x - cx) * (x - cx));
        boden[x] = max((int)boden[x], min(FY + FH - 4, cy + dy));
    }
}

static void feuer() {
    if (vorbei) return;
    int ex, ey;
    int treffer = simuliere(am, winkel[am], kraft[am], ex, ey, true);
    const char* wer = am == 0 ? T("Voll-Fahne", "Solid flag") : (gegenSticky ? "Sticky" : T("Ring-Fahne", "Ring flag"));
    if (treffer >= 0) {
        hp[treffer] = max(0, hp[treffer] - 1);
        krater(ex, boden[burgX[treffer]], 10);
        if (hp[treffer] == 0) { vorbei = true; snprintf(meldung, sizeof meldung, T("%s gewinnt!", "%s wins!"), treffer == 0 ? (gegenSticky ? "Sticky" : T("Ring-Fahne", "Ring flag")) : T("Voll-Fahne", "Solid flag")); }
        else snprintf(meldung, sizeof meldung, T("%s trifft!", "%s hits!"), wer);
    } else if (treffer == -1) { krater(ex, ey, 16); snprintf(meldung, sizeof meldung, T("%s: daneben", "%s: missed"), wer); }
    else snprintf(meldung, sizeof meldung, T("%s: ins Aus", "%s: out"), wer);
    zuege++;
    if (!vorbei) { am = 1 - am; int dw = random(-2, 3); wind = constrain(wind + dw, -6, 6); }   // random() nie in constrain(): Makro
    allesNeu();
    uint32_t ms = teil(0, 0, W, H);
    messung("burgduell", "zug", ms, "ms", meldung);
}

/** Der Sticky zielt: probiert Kräfte, nimmt die beste, mit Streuung, die mit jedem Zug kleiner wird */
static void stickyZieht() {
    int beste = 55, w = random(35, 60), bestAbst = 9999;
    for (int k = 15; k <= 100; k++) {
        int ex, ey; int t = simuliere(1, w, k, ex, ey, false);
        int abst = t == 0 ? 0 : (t == -2 ? 9999 : abs(ex - burgX[0]));   // ins Aus zählt nie als nah
        if (abst < bestAbst) { bestAbst = abst; beste = k; }
    }
    int streu = max(2, 14 - zuege);
    winkel[1] = w; int dk = random(-streu, streu + 1); kraft[1] = constrain(beste + dk, 10, 100);
    snprintf(meldung, sizeof meldung, "%s", T("Sticky zielt ...", "Sticky aims ..."));
    hud(); teil(0, HUD_Y, W, H - HUD_Y);
    delay(400);
    feuer();
}

// ---- Bedienung ----------------------------------------------------------------------------------------------
static void betreten() { if (!zuege && !spurN) neueWelt(); allesNeu(); }
static void taste(Knopf k) {
    if (vorbei || (gegenSticky && am == 1)) return;
    winkel[am] = constrain(winkel[am] + (k == K_UP ? 3 : -3), 5, 85);
    feldZeichnen(true); hud(); teil(0, FY, W, H - FY);
}
static void tipp(int x, int y) {
    if (vorbei) { neueWelt(); allesNeu(); teil(0, 0, W, H); return; }
    if (drin(x, y, GX, GY, GW, GH)) { gegenSticky = !gegenSticky; hud(); teil(0, HUD_Y, W, H - HUD_Y); return; }
    auto rund = [&](int cx) { int a = x - cx, b = y - RY; return a * a + b * b <= (RR + 10) * (RR + 10); };
    if (!(gegenSticky && am == 1) && y >= HUD_Y) {   // Regler: Winkel ±5°, Pulver ±5
        int w = winkel[am], k = kraft[am];
        if (rund(WM_X)) w -= 5; else if (rund(WP_X)) w += 5; else if (rund(PM_X)) k -= 5; else if (rund(PP_X)) k += 5;
        w = constrain(w, 5, 85); k = constrain(k, 10, 100);
        if (w != winkel[am] || k != kraft[am]) {
            winkel[am] = w; kraft[am] = k;
            feldZeichnen(true); hud(); teil(0, FY, W, H - FY);
            return;
        }
    }
    int dx = x - FEUER_X, dy = y - FEUER_Y;
    if (dx * dx + dy * dy <= (FEUER_R + 8) * (FEUER_R + 8)) {
        feuer();
        if (gegenSticky && am == 1 && !vorbei) stickyZieht();
        return;
    }
    if (y < HUD_Y && !(gegenSticky && am == 1)) {   // zielen: Richtung und Abstand zur eigenen Kanone
        int kx, ky; kanone(am, kx, ky);
        float ddx = (x - kx) * (am == 0 ? 1 : -1), ddy = ky - y;
        winkel[am] = constrain((int)lroundf(atan2f(ddy, max(ddx, 1.0f)) * RAD_TO_DEG), 5, 85);
        kraft[am] = constrain((int)(sqrtf(ddx * ddx + ddy * ddy) / 2.6f), 10, 100);
        snprintf(meldung, sizeof meldung, "%s", T("zielt", "aiming"));
        feldZeichnen(true); hud(); teil(0, FY, W, H - FY);
    }
}

}  // namespace burgduell
