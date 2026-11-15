// Station 5: Bewegung – wie viel Animation verträgt e-Ink?
// Drei Animationen (Balken, Zeiger, Zahlenrolle), je Bild ein Teil-Refresh nur des bewegten Bereichs.
// Schalter „wach“ = Panel zwischen den Bildern nicht schlafen legen (Bibliotheks-Patch). „Los“ spielt sie ab und
// misst Bilder je Sekunde. KI bricht ab.
#pragma once
#include "../braun.h"

namespace bewegung {

enum Anim : uint8_t { A_BALKEN, A_ZEIGER, A_ROLLE, A_ANZAHL };
static const char* A_NAME[A_ANZAHL] = { "Balken", "Zeiger", "Rolle" };
static const char* A_CSV[A_ANZAHL] = { "anim-balken", "anim-zeiger", "anim-rolle" };
static Anim anim = A_BALKEN;
static bool wach = false;
static const int BUEHNE_Y = 60, BUEHNE_H = 270;
static const int PY = 404, PH = 58, PW = 130, PGAP = 8, PX0 = 16;
static const int SWX = 440, SWY = 404, SWW = 110, SWH = 58;
static const int LOS_X = 700, LOS_Y = 424, LOS_R = 46;
static const int ERG_Y = 350;
static char ergebnis[96] = "";

// Jede Animation: Bild k von n in den Puffer zeichnen und den bewegten Bereich nennen
static void bildBalken(int k, int n, int& x, int& y, int& w, int& h) {
    x = 100; y = 170; w = 600; h = 44;
    display.fillRect(x, y, w, h, WEISS);
    ringRR(x, y, w, h, h / 2, 3);
    int f = (w - 12) * k / n;
    if (f > 0) display.fillRoundRect(x + 6, y + 6, max(f, h - 12), h - 12, (h - 12) / 2, SCHWARZ);
}
static void bildZeiger(int k, int n, int& x, int& y, int& w, int& h) {
    int cx = W / 2, cy = 195, r = 100;
    x = cx - r - 4; y = cy - r - 4; w = 2 * r + 8; h = 2 * r + 8;
    display.fillRect(x, y, w, h, WEISS);
    for (int i = 0; i < 3; i++) display.drawCircle(cx, cy, r - i, SCHWARZ);
    for (int i = 0; i < 12; i++) {
        float a = i * 30 * DEG_TO_RAD - HALF_PI;
        display.drawWideLine(cx + cosf(a) * (r - 14), cy + sinf(a) * (r - 14), cx + cosf(a) * (r - 4), cy + sinf(a) * (r - 4), 3, SCHWARZ);
    }
    float a = k * 360.0f / n * DEG_TO_RAD - HALF_PI;
    display.drawWideLine(cx, cy, cx + cosf(a) * (r - 20), cy + sinf(a) * (r - 20), 6, SCHWARZ);
    display.fillCircle(cx, cy, 9, SCHWARZ);
}
static void bildRolle(int k, int, int& x, int& y, int& w, int& h) {
    x = 250; y = 120; w = 300; h = 150;
    display.fillRect(x, y, w, h, WEISS);
    ringRR(x, y, w, h, 20, 3);
    char b[8]; snprintf(b, sizeof b, "%04d", k);
    display.setFreeFont(&FreeSansBold24pt7b); display.setTextSize(2); display.setTextColor(SCHWARZ);
    display.setTextDatum(MC_DATUM); display.drawString(b, x + w / 2, y + h / 2);
    display.setTextSize(1);
}
static void bild(int k, int n, int& x, int& y, int& w, int& h) {
    if (anim == A_BALKEN) bildBalken(k, n, x, y, w, h);
    else if (anim == A_ZEIGER) bildZeiger(k, n, x, y, w, h);
    else bildRolle(k, n, x, y, w, h);
}

static void unten() {
    display.fillRect(0, ERG_Y - 16, W, H - ERG_Y + 16, WEISS);
    text(*ergebnis ? ergebnis : "Los spielt 12 Bilder ab und misst. wach = Panel bleibt zwischen den Bildern wach.",
         W / 2, ERG_Y, &FreeSans9pt7b, MC_DATUM);
    for (int i = 0; i < A_ANZAHL; i++) pille(PX0 + i * (PW + PGAP), PY, PW, PH, A_NAME[i], i == anim ? R_AUSWAHL : R_NEBEN);
    schalter(SWX, SWY, SWW, SWH, wach);
    text("wach", SWX + SWW / 2, SWY - 12, &FreeSans9pt7b, MC_DATUM);
    doppelring(LOS_X, LOS_Y, LOS_R, "Los");
}
static void buehne() {
    display.fillRect(0, BUEHNE_Y, W, BUEHNE_H, WEISS);
    int x, y, w, h; bild(0, 12, x, y, w, h);
}
static void betreten() { statuszeile("5 · Bewegung"); buehne(); unten(); }

static void abspielen() {
    const int n = 12;
    uint32_t t0 = millis(), summe = 0;
    int k = 1;
    for (; k <= n; k++) {
        if (tasteUnten(K_KI)) break;
        int x, y, w, h;
        bild(k, n, x, y, w, h);
        summe += teil(x, y, w, h, wach);
    }
    int bilder = k - 1;
    float s = (millis() - t0) / 1000.0f;
    if (bilder > 0) {
        snprintf(ergebnis, sizeof ergebnis, "%s%s: %d Bilder in %.1f s = %.2f Bilder/s (%lu ms je Teil-Refresh)",
                 A_NAME[anim], wach ? " wach" : "", bilder, s, bilder / s, (unsigned long)(summe / bilder));
        char notiz[40]; snprintf(notiz, sizeof notiz, "%.2f bilder/s%s", bilder / s, wach ? " wach" : "");
        messung("bewegung", A_CSV[anim], summe / bilder, "ms", notiz);
    }
    display.fillRect(0, ERG_Y - 16, W, 32, WEISS);
    text(ergebnis, W / 2, ERG_Y, &FreeSans9pt7b, MC_DATUM);
    teil(0, ERG_Y - 16, W, 32);
}
static void waehle(int a) { anim = (Anim)((a + A_ANZAHL) % A_ANZAHL); buehne(); unten(); teil(0, BUEHNE_Y, W, H - BUEHNE_Y); }
static void taste(Knopf k) { waehle(anim + (k == K_DOWN ? 1 : -1)); }
static void tipp(int x, int y) {
    int dx = x - LOS_X, dy = y - LOS_Y;
    if (dx * dx + dy * dy <= LOS_R * LOS_R) { abspielen(); return; }
    for (int i = 0; i < A_ANZAHL; i++) if (drin(x, y, PX0 + i * (PW + PGAP), PY, PW, PH)) { waehle(i); return; }
    if (drin(x, y, SWX, SWY, SWW, SWH)) { wach = !wach; schalter(SWX, SWY, SWW, SWH, wach); teil(SWX, SWY, SWW, SWH); }
}

}  // namespace bewegung
