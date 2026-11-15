// Station 6: Touch – wie genau trifft der Finger, wie schnell sieht man die Rückmeldung, wie flüssig kritzelt es?
// Drei Versuche, umschaltbar mit ▲/▼ oder den Pillen unten:
//   Zielen       9 Ziele bis an den Rand, Abweichung in px und mm, systematischer Versatz
//   Rückmeldung  4 Arten (Rahmen, Invertieren, Punkt, nur Klick): Zeit von der Berührung bis das Bild steht
//   Kritzeln     Linie folgt dem Finger, Teil-Refresh des berührten Bereichs; erreichte Teilbilder je Sekunde
#pragma once

namespace touch {

static const int AY = STATUS_H + 4, AH = 386 - AY;                  // Testfläche (ganze Breite)
static const int PY = 400, PH = 58, PW = 172, PGAP = 10, PX0 = 16;  // Pillen
enum Modus : uint8_t { M_ZIELEN, M_RUECK, M_KRITZELN, M_ANZAHL };
static const char* M_NAME[M_ANZAHL] = { "Zielen", "Rückmeldung", "Kritzeln" };
static Modus modus = M_ZIELEN;

static void pillen() {
    display.fillRect(0, PY - 4, W, PH + 8, WEISS);
    for (int i = 0; i < M_ANZAHL; i++) pille(PX0 + i * (PW + PGAP), PY, PW, PH, M_NAME[i], i == modus ? R_AUSWAHL : R_NEBEN);
    pille(W - 16 - PW, PY, PW, PH, modus == M_KRITZELN ? "Löschen" : "Neu", R_WERKZEUG);
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
    text("Tippe jeweils genau in die Mitte des Ziels (9 Ziele)", W / 2, AY + AH / 2 + 48, &FreeSans9pt7b, MC_DATUM);
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
    text("Zielen: Ergebnis", 30, AY + 14, &FreeSansBold12pt7b);
    snprintf(t, sizeof t, "Abweichung im Mittel %.1f px (%.1f mm), höchstens %.0f px (%.1f mm)", sb / 9, sb / 9 / mm, mx, mx / mm);
    text(t, 30, AY + 60, &FreeSans12pt7b);
    snprintf(t, sizeof t, "Versatz: x %+.1f px, y %+.1f px (+ = rechts/unten)", sx / 9, sy / 9);
    text(t, 30, AY + 100, &FreeSans12pt7b);
    // Streubild: alle Treffer relativ zur Zielmitte, 4-fach vergrößert
    int cx = W - 150, cy = AY + AH / 2 + 10;
    display.drawCircle(cx, cy, ZR * 4, SCHWARZ);
    display.drawFastHLine(cx - ZR * 4 - 8, cy, ZR * 8 + 16, SCHWARZ);
    display.drawFastVLine(cx, cy - ZR * 4 - 8, ZR * 8 + 16, SCHWARZ);
    for (int i = 0; i < 9; i++) display.fillCircle(cx + zdx[i] * 4, cy + zdy[i] * 4, 4, SCHWARZ);
    text("Streubild x4", cx, cy + ZR * 4 + 22, &FreeSans9pt7b, MC_DATUM);
    text("Neu = noch einmal", 30, AY + 150, &FreeSans9pt7b);
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
static const char* R_NAME[R_ANZAHL] = { "Rahmen", "Invertieren", "Punkt", "nur Klick" };
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
    text("Zeit = Berührung erkannt bis Bild steht (dazu kommen bis ~35 ms Abfrage des Touch-Chips)",
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

static void kritzelnAnfang() {
    flaecheLeer();
    display.drawRect(0, AY, W, AH, SCHWARZ);
    text("Mit dem Finger zeichnen. Löschen = leeren", 12, AY + 8, &FreeSans9pt7b);
    strichAn = false; dreckig = false;
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
        if (drin(x, y, W - 16 - PW, PY, PW, PH)) { modusAnfang(); teil(0, AY, W, AH); }
        return;
    }
    if (modus == M_ZIELEN) zielenTipp(x, y);
    else if (modus == M_RUECK) rueckTipp(x, y, t0);
}

}  // namespace touch
