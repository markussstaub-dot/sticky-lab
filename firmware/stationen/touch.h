// Station 6: Touch – wie genau trifft der Finger, wie schnell sieht man die Rückmeldung, wie flüssig kritzelt es?
// Drei Versuche, umschaltbar mit ▲/▼ oder den Pillen unten:
//   Zielen       9 Ziele bis an den Rand, Abweichung in px und mm, systematischer Versatz
//   Rückmeldung  4 Arten (Rahmen, Invertieren, Punkt, nur Klick): Zeit von der Berührung bis das Bild steht
//   Kritzeln     Linie folgt dem Finger, Teil-Refresh des berührten Bereichs; erreichte Teilbilder je Sekunde
#pragma once

namespace touch {

static const int AY = STATUS_H + 4, AH = 386 - AY;                  // Testfläche (ganze Breite)
static const int PY = 400, PH = 58, PW = 164, PGAP = 8, PX0 = 12;  // Pillen
enum Modus : uint8_t { M_ZIELEN, M_RUECK, M_KRITZELN, M_ANZAHL };
static const char* M_NAME_DE[M_ANZAHL] = { "Zielen", "Rückmeldung", "Kritzeln" };
static const char* M_NAME_EN[M_ANZAHL] = { "Aim", "Feedback", "Scribble" };
#define M_NAME (EN ? M_NAME_EN : M_NAME_DE)
static Modus modus = M_ZIELEN;
enum Vorlage : uint8_t { VL_FREI, VL_SPIRALE, VL_LABY, VL_ANZAHL };
static uint8_t vorlage = VL_FREI;   // Kritzeln: Vorlage zum Nachfahren
static const char* VL_NAME_DE[VL_ANZAHL] = { "frei", "Spirale", "Labyrinth" };
static const char* VL_NAME_EN[VL_ANZAHL] = { "free", "Spiral", "Maze" };
#define VL_NAME (EN ? VL_NAME_EN : VL_NAME_DE)
static const int KPW = 116, KL_X = W - 16 - KPW, KV_X = KL_X - 10 - KPW;

static void pillen() {
    display.fillRect(0, PY - 4, W, PH + 8, WEISS);
    for (int i = 0; i < M_ANZAHL; i++) pille(PX0 + i * (PW + PGAP), PY, PW, PH, M_NAME[i], i == modus ? R_AUSWAHL : R_NEBEN);
    if (modus == M_KRITZELN) {   // zwei kleine Pillen: Vorlage umschalten, Löschen
        pille(KV_X, PY, KPW, PH, VL_NAME[vorlage], vorlage ? R_AUSWAHL : R_NEBEN);
        pille(KL_X, PY, KPW, PH, T("Löschen", "Clear"), R_WERKZEUG);
    } else pille(W - 16 - PW, PY, PW, PH, T("Neu", "New"), R_WERKZEUG);
}
static void flaecheLeer() { display.fillRect(0, AY, W, AH, WEISS); }

// ---- Zielen -----------------------------------------------------------------------------------------
static const int ZR = 22;   // Zielradius
static const int ZX[3] = { 30, W / 2, W - 30 }, ZY[3] = { AY + 28, AY + AH / 2, AY + AH - 28 };
static int zielNr = 0;
static int zdx[9], zdy[9];

static void zielLage(int k, int& x, int& y) {
    static const int folge[9] = { 4, 0, 8, 2, 6, 1, 7, 3, 5 };   // erst Mitte, dann Ecken, dann Kanten
    int f = folge[k]; x = ZX[f % 3]; y = ZY[f / 3];
}
static void zielZeichnen(int x, int y) {
    display.drawCircle(x, y, ZR, SCHWARZ); display.drawCircle(x, y, ZR - 1, SCHWARZ);
    display.drawFastHLine(x - ZR - 6, y, 2 * ZR + 12, SCHWARZ);
    display.drawFastVLine(x, y - ZR - 6, 2 * ZR + 12, SCHWARZ);
    display.fillCircle(x, y, 3, SCHWARZ);
}
static void zielBox(int x, int y, int& bx, int& by, int& bw, int& bh) {
    int r = ZR + 12;
    bx = max(0, x - r); by = max(AY, y - r); bw = min(W, x + r) - bx; bh = min(AY + AH, y + r) - by;
}
static void zielenAnfang() {
    zielNr = 0;
    flaecheLeer();
    text(T("Tippe jeweils genau in die Mitte des Ziels (9 Ziele)", "Tap exactly the centre of each target (9 targets)"), W / 2, AY + AH / 2 + 48, &FreeSans9pt7b, MC_DATUM);
    int x, y; zielLage(0, x, y); zielZeichnen(x, y);
}
static void zielenErgebnis() {
    flaecheLeer();
    double sx = 0, sy = 0, sb = 0, mx = 0;
    for (int i = 0; i < 9; i++) {
        sx += zdx[i]; sy += zdy[i];
        double b = sqrt((double)zdx[i] * zdx[i] + (double)zdy[i] * zdy[i]);
        sb += b; if (b > mx) mx = b;
    }
    char t[96];
    const float mm = PANEL.pxProMm;
    text(T("Zielen: Ergebnis", "Aim: result"), 30, AY + 14, &FreeSansBold12pt7b);
    snprintf(t, sizeof t, T("Abweichung im Mittel %.1f px (%.1f mm), höchstens %.0f px (%.1f mm)", "Mean error %.1f px (%.1f mm), max %.0f px (%.1f mm)"), sb / 9, sb / 9 / mm, mx, mx / mm);
    text(t, 30, AY + 60, &FreeSans12pt7b);
    snprintf(t, sizeof t, T("Versatz: x %+.1f px, y %+.1f px (+ = rechts/unten)", "Offset: x %+.1f px, y %+.1f px (+ = right/down)"), sx / 9, sy / 9);
    text(t, 30, AY + 100, &FreeSans12pt7b);
    // Streubild: alle Treffer relativ zur Zielmitte, 4-fach vergrößert
    int cx = W - 150, cy = AY + AH / 2 + 10;
    display.drawCircle(cx, cy, ZR * 4, SCHWARZ);
    display.drawFastHLine(cx - ZR * 4 - 8, cy, ZR * 8 + 16, SCHWARZ);
    display.drawFastVLine(cx, cy - ZR * 4 - 8, ZR * 8 + 16, SCHWARZ);
    for (int i = 0; i < 9; i++) display.fillCircle(cx + zdx[i] * 4, cy + zdy[i] * 4, 4, SCHWARZ);
    text(T("Streubild x4", "Scatter x4"), cx, cy + ZR * 4 + 22, &FreeSans9pt7b, MC_DATUM);
    text(T("Neu = noch einmal", "New = again"), 30, AY + 150, &FreeSans9pt7b);
    teil(0, AY, W, AH);
    messung("touch", "ziel-mittel", lround(sb / 9 * 10), "0.1px");
    messung("touch", "ziel-max", lround(mx * 10), "0.1px");
    messung("touch", "versatz-x", lround(sx / 9 * 10), "0.1px");
    messung("touch", "versatz-y", lround(sy / 9 * 10), "0.1px");
}
static void zielenTipp(int x, int y) {
    if (zielNr >= 9) return;
    int tx, ty; zielLage(zielNr, tx, ty);
    zdx[zielNr] = x - tx; zdy[zielNr] = y - ty;
    zielNr++;
    if (zielNr == 9) { zielenErgebnis(); return; }
    // altes Ziel weg (Treffer als Kreuz stehen lassen), neues Ziel zeichnen: zwei kleine Teil-Refreshes
    int bx, by, bw, bh;
    zielBox(tx, ty, bx, by, bw, bh);
    display.fillRect(bx, by, bw, bh, WEISS);
    display.drawLine(x - 5, y - 5, x + 5, y + 5, SCHWARZ); display.drawLine(x - 5, y + 5, x + 5, y - 5, SCHWARZ);
    if (zielNr == 1) {   // erstes Ziel war die Mitte: Hinweiszeile darunter gleich mit weg
        display.fillRect(0, AY + AH / 2 + 36, W, 26, WEISS);
        int y2 = max(by + bh, AY + AH / 2 + 62);
        bx = 0; bw = W; bh = y2 - by;
    }
    teil(bx, by, bw, bh);
    int nx, ny; zielLage(zielNr, nx, ny);
    zielZeichnen(nx, ny);
    zielBox(nx, ny, bx, by, bw, bh);
    teil(bx, by, bw, bh);
}

// ---- Rückmeldung ------------------------------------------------------------------------------------
enum Rueck : uint8_t { R_RAHMEN, R_INVERT, R_PUNKT, R_KLICK, R_ANZAHL };
static const char* R_NAME_DE[R_ANZAHL] = { "Rahmen", "Invertieren", "Punkt", "nur Klick" };
static const char* R_NAME_EN[R_ANZAHL] = { "Frame", "Invert", "Dot", "click only" };
#define R_NAME (EN ? R_NAME_EN : R_NAME_DE)
static const char* R_CSV[R_ANZAHL] = { "rueck-rahmen", "rueck-invert", "rueck-punkt", "rueck-klick" };
static const int RW = 180, RH = 150, RGAP = 13, RX0 = 16, RY = AY + 30;
static long rueckMs[R_ANZAHL] = { -1, -1, -1, -1 };

static void rueckFeld(int i, bool aktiv) {
    int x = RX0 + i * (RW + RGAP);
    display.fillRect(x, RY, RW, RH + 40, WEISS);
    if (aktiv && i == R_INVERT) display.fillRoundRect(x, RY, RW, RH, 18, SCHWARZ);
    else ringRR(x, RY, RW, RH, 18, aktiv && i == R_RAHMEN ? 8 : 2);
    text(R_NAME[i], x + RW / 2, RY + RH / 2, &FreeSans12pt7b, MC_DATUM, aktiv && i == R_INVERT ? WEISS : SCHWARZ);
    char b[24];
    if (rueckMs[i] >= 0) snprintf(b, sizeof b, "%ld ms", rueckMs[i]); else snprintf(b, sizeof b, "-");
    text(b, x + RW / 2, RY + RH + 20, &FreeSansBold12pt7b, MC_DATUM);
}
static void rueckAnfang() {
    flaecheLeer();
    for (int i = 0; i < R_ANZAHL; i++) rueckFeld(i, false);
    text(T("Zeit = Berührung erkannt bis Bild steht (dazu kommen bis ~35 ms Abfrage des Touch-Chips)", "Time = touch detected until the image is done (plus up to ~35 ms touch polling)"),
         W / 2, AY + 8, &FreeSans9pt7b, TC_DATUM);
}
static void rueckTipp(int x, int y, uint32_t t0) {
    for (int i = 0; i < R_ANZAHL; i++) {
        int fx = RX0 + i * (RW + RGAP);
        if (!drin(x, y, fx, RY, RW, RH)) continue;
        uint32_t ms = 0;
        if (i == R_PUNKT) {
            display.fillCircle(x, y, 12, SCHWARZ);
            teil(x - 14, y - 14, 28, 28);
            ms = millis() - t0;
        } else if (i == R_KLICK) {
            ms = millis() - t0;   // der Klick kam schon beim Erkennen (eingabe.h), kein Refresh
        } else {
            rueckFeld(i, true);
            teil(fx, RY, RW, RH);
            ms = millis() - t0;
        }
        rueckMs[i] = ms;
        char notiz[32];
        snprintf(notiz, sizeof notiz, "%dx%d", i == R_PUNKT ? 28 : RW, i == R_PUNKT ? 28 : RH);
        messung("touch", R_CSV[i], ms, "ms", i == R_KLICK ? "ohne refresh" : notiz);
        delay(250);
        rueckFeld(i, false);   // zurück, Zeit steht jetzt darunter
        teil(fx, RY, RW, RH + 40);
        return;
    }
}

// ---- Kritzeln ---------------------------------------------------------------------------------------
static bool strichAn = false;
static int lx, ly;                              // letzter Punkt
static int dx0, dy0, dx1, dy1; static bool dreckig = false;   // noch nicht gezeigter Bereich
static uint32_t letzterRefresh = 0, strichStart = 0, strichRefreshs = 0, strichMs = 0;

// Spirale (archimedisch, 3 Windungen) als Vorlage: Punkte vorberechnet, Abweichung = Abstand zum nächsten Punkt
static const int SPN = 600;
static int16_t spX[SPN], spY[SPN];
static double abwSumme = 0; static uint32_t abwN = 0;
static void spiraleRechnen() {
    const float cx = W / 2, cy = AY + AH / 2 + 6, rMax = AH / 2 - 22, wind = 3;
    for (int i = 0; i < SPN; i++) {
        float t = (float)i / (SPN - 1), th = t * wind * TWO_PI, r = 8 + t * (rMax - 8);
        spX[i] = cx + r * cosf(th); spY[i] = cy + r * sinf(th);
    }
}
static float spiraleAbstand(int x, int y) {
    long best = 1L << 30;
    for (int i = 0; i < SPN; i++) { long dx = x - spX[i], dy = y - spY[i]; long d = dx * dx + dy * dy; if (d < best) best = d; }
    return sqrtf((float)best);
}

static void dazu(int x, int y);   // weiter unten: Bereich für den nächsten Teil-Refresh merken
// Labyrinth (Tiefensuche mit Zufall): Start links oben, Ziel rechts unten. Gemessen: Zeit bis zum Ziel und
// wie oft der Finger eine Wand berührt (näher als 3 px an einer Wandlinie).
static const int LC = 36, LCOLS = 21, LROWS = 8;
static const int LX0 = (W - LC * LCOLS) / 2, LY0 = AY + 34;
static uint8_t lwand[LROWS][LCOLS];   // Bits: 1 = oben, 2 = rechts, 4 = unten, 8 = links
static bool labyLaeuft = false, labyFertig = false, anWand = false;
static uint32_t labyStart = 0, wandTreffer = 0;
static void labyBauen() {
    static bool besucht[LROWS][LCOLS];
    static int16_t stapel[LROWS * LCOLS];
    memset(besucht, 0, sizeof besucht);
    memset(lwand, 15, sizeof lwand);
    int n = 0; stapel[n++] = 0; besucht[0][0] = true;
    while (n) {
        int c = stapel[n - 1], r = c / LCOLS, k = c % LCOLS;
        int frei[4], f = 0;
        const int dr[4] = { -1, 0, 1, 0 }, dk[4] = { 0, 1, 0, -1 };
        for (int d = 0; d < 4; d++) {
            int rr = r + dr[d], kk = k + dk[d];
            if (rr >= 0 && rr < LROWS && kk >= 0 && kk < LCOLS && !besucht[rr][kk]) frei[f++] = d;
        }
        if (!f) { n--; continue; }
        int d = frei[random(f)], rr = r + dr[d], kk = k + dk[d];
        lwand[r][k] &= ~(1 << d); lwand[rr][kk] &= ~(1 << ((d + 2) % 4));
        besucht[rr][kk] = true; stapel[n++] = rr * LCOLS + kk;
    }
    labyLaeuft = labyFertig = anWand = false; wandTreffer = 0;
}
static void labyZeichnen() {
    for (int r = 0; r < LROWS; r++) for (int k = 0; k < LCOLS; k++) {
        int x = LX0 + k * LC, y = LY0 + r * LC;
        if (lwand[r][k] & 1) display.fillRect(x, y, LC + 2, 2, SCHWARZ);
        if (lwand[r][k] & 2) display.fillRect(x + LC, y, 2, LC + 2, SCHWARZ);
        if (lwand[r][k] & 4) display.fillRect(x, y + LC, LC + 2, 2, SCHWARZ);
        if (lwand[r][k] & 8) display.fillRect(x, y, 2, LC + 2, SCHWARZ);
    }
    display.drawCircle(LX0 + LC / 2, LY0 + LC / 2, 8, SCHWARZ);                       // Start: Ring
    display.fillCircle(LX0 + (LCOLS - 1) * LC + LC / 2, LY0 + (LROWS - 1) * LC + LC / 2, 8, SCHWARZ);   // Ziel: Punkt
}
static void labyPunkt(int x, int y) {
    if (labyFertig) return;
    int k = (x - LX0) / LC, r = (y - LY0) / LC;
    if (x < LX0 || y < LY0 || k >= LCOLS || r >= LROWS) return;
    if (!labyLaeuft) { if (r == 0 && k == 0) { labyLaeuft = true; labyStart = millis(); } else return; }
    int ux = x - LX0 - k * LC, uy = y - LY0 - r * LC, w = lwand[r][k];
    bool trifft = ((w & 1) && uy < 3) || ((w & 2) && ux > LC - 3) || ((w & 4) && uy > LC - 3) || ((w & 8) && ux < 3);
    if (trifft && !anWand) wandTreffer++;
    anWand = trifft;
    if (r == LROWS - 1 && k == LCOLS - 1) {
        labyFertig = true;
        float s = (millis() - labyStart) / 1000.0f;
        char b[64]; snprintf(b, sizeof b, T("Ziel in %.1f s, %lu Wandberührungen", "Goal in %.1f s, %lu wall touches"), s, (unsigned long)wandTreffer);
        display.fillRect(W / 2, AY + 2, W / 2 - 2, 24, WEISS);
        text(b, W - 12, AY + 8, &FreeSans9pt7b, TR_DATUM);
        dazu(W / 2, AY + 2); dazu(W - 4, AY + 26);
        char n[32]; snprintf(n, sizeof n, "%lu wand", (unsigned long)wandTreffer);
        messung("touch", "labyrinth", lroundf(s * 10), "0.1s", n);
    }
}
static void kritzelnAnfang() {
    flaecheLeer();
    display.drawRect(0, AY, W, AH, SCHWARZ);
    if (vorlage == VL_LABY) {
        labyBauen(); labyZeichnen();
        text(T("Vom Ring zum Punkt, ohne die Wände zu berühren", "From the ring to the dot without touching walls"), 12, AY + 8, &FreeSans9pt7b);
    } else if (vorlage == VL_SPIRALE) {
        spiraleRechnen();
        for (int i = 0; i + 1 < SPN; i++) if (i % 3 != 2) display.drawLine(spX[i], spY[i], spX[i + 1], spY[i + 1], SCHWARZ);   // gestrichelt
        text(T("Spirale von innen nach außen nachfahren", "Trace the spiral from the inside out"), 12, AY + 8, &FreeSans9pt7b);
    } else text(T("Mit dem Finger zeichnen. Löschen = leeren", "Draw with your finger. Clear = empty"), 12, AY + 8, &FreeSans9pt7b);
    strichAn = false; dreckig = false; abwSumme = 0; abwN = 0;
}
static void dazu(int x, int y) {
    if (!dreckig) { dx0 = dx1 = x; dy0 = dy1 = y; dreckig = true; }
    dx0 = min(dx0, x); dy0 = min(dy0, y); dx1 = max(dx1, x); dy1 = max(dy1, y);
}
/** Jeder Punkt aus dem Touch-Sammler (eingabe.h), auch die während eines Refreshs gesammelten */
static void punkt(int x, int y, bool neu) {
    if (modus != M_KRITZELN || !drin(x, y, 0, AY, W, AH)) return;
    if (neu || !strichAn) {
        if (!strichAn) { strichStart = millis(); strichRefreshs = 0; strichMs = 0; }
        strichAn = true; lx = x; ly = y;
        display.fillCircle(x, y, 2, SCHWARZ); dazu(x, y);
        return;
    }
    display.drawWideLine(lx, ly, x, y, 4, SCHWARZ);
    dazu(lx, ly); dazu(x, y);
    lx = x; ly = y;
    if (vorlage == VL_SPIRALE) { abwSumme += spiraleAbstand(x, y); abwN++; }
    if (vorlage == VL_LABY) labyPunkt(x, y);
}
static void kritzelnTick() {
    if (modus != M_KRITZELN) return;
    tpPunkt = punkt;
    if (dreckig && millis() - letzterRefresh > 60) {
        int m = 5;
        int x = max(0, dx0 - m), y = max(AY, dy0 - m), x2 = min(W, dx1 + m), y2 = min(AY + AH, dy1 + m);
        dreckig = false;
        strichMs += teil(x, y, x2 - x, y2 - y, true);   // wach halten wie der EDC-Kritzelblock
        strichRefreshs++;
        letzterRefresh = millis();
    }
    if (strichAn && !tpUnten && !dreckig) {
        strichAn = false;
        if (strichRefreshs) {
            float s = (millis() - strichStart) / 1000.0f;
            char notiz[48];
            snprintf(notiz, sizeof notiz, "%.1f bilder/s, %.1fs", strichRefreshs / max(s, 0.1f), s);
            messung("touch", "kritzel-teil", strichMs / strichRefreshs, "ms", notiz);
        }
        if (vorlage == VL_SPIRALE && abwN > 20) {   // Ergebnis oben rechts einblenden
            float m = abwSumme / abwN;
            char b[64]; snprintf(b, sizeof b, T("Abweichung %.1f px (%.1f mm), %lu Punkte", "Error %.1f px (%.1f mm), %lu points"), m, m / PANEL.pxProMm, (unsigned long)abwN);
            display.fillRect(W / 2, AY + 2, W / 2 - 2, 24, WEISS);
            text(b, W - 12, AY + 8, &FreeSans9pt7b, TR_DATUM);
            teil(W / 2, AY + 2, W / 2 - 2, 24);
            char n[24]; snprintf(n, sizeof n, "%lu punkte", (unsigned long)abwN);
            messung("touch", "spirale", lroundf(m * 10), "0.1px", n);
        }
    }
}

// ---- Rahmen -------------------------------------------------------------------------------------------
static void modusAnfang() {
    if (modus == M_ZIELEN) zielenAnfang();
    else if (modus == M_RUECK) rueckAnfang();
    else kritzelnAnfang();
}
static void betreten() {
    tpPunkt = punkt;
    statuszeile("6 · Touch");
    modusAnfang();
    pillen();
}
static void wechsel(int m) {
    modus = (Modus)((m + M_ANZAHL) % M_ANZAHL);
    modusAnfang();
    pillen();
    teil(0, AY, W, H - AY);
}
static void taste(Knopf k) {
    if (k == K_UP) wechsel(modus - 1);
    if (k == K_DOWN) wechsel(modus + 1);
}
static void tipp(int x, int y) {
    uint32_t t0 = millis();
    if (y >= PY - 4) {
        for (int i = 0; i < M_ANZAHL; i++) if (drin(x, y, PX0 + i * (PW + PGAP), PY, PW, PH)) { wechsel(i); return; }
        if (modus == M_KRITZELN) {
            if (drin(x, y, KV_X, PY, KPW, PH)) vorlage = (vorlage + 1) % VL_ANZAHL;
            else if (!drin(x, y, KL_X, PY, KPW, PH)) return;
            modusAnfang(); pillen(); teil(0, AY, W, H - AY);
        } else if (drin(x, y, W - 16 - PW, PY, PW, PH)) { modusAnfang(); teil(0, AY, W, AH); }
        return;
    }
    if (modus == M_ZIELEN) zielenTipp(x, y);
    else if (modus == M_RUECK) rueckTipp(x, y, t0);
}

}  // namespace touch
