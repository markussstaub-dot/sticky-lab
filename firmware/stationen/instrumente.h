// Station 7: Instrumente – die eingebauten Sensoren als Braun-Instrumente, live
// Klima (SHT40, mit Taupunkt), Wasserwaage (LSM6DS3TR-C), Uhr (PCF8563), Akku (BQ27220).
// Alle 10 s ein Teil-Refresh der Anzeigen (kostet Strom: dafür ist Station 8 da), Tippen = sofort.
// Jede Minute eine Klima-Zeile in /lab/messungen.csv (Messreihe, z. B. Innenraum Microlino).
#pragma once
#include "../sensoren.h"

namespace instrumente {

static const int IY = 70, IH = 330;                 // Instrumentenzeile
static const int CX[4] = { 100, 300, 500, 700 }, CY = 190, R = 82;
static uint32_t zuletzt = 0, zuletztCsv = 0;

static void skala(int cx, int cy, int r, int n, float a0, float a1) {
    for (int i = 0; i < n; i++) {
        float a = a0 + (a1 - a0) * i / (n - 1);
        int l = (i % 5 == 0) ? 12 : 6;
        display.drawWideLine(cx + cosf(a) * (r - l), cy + sinf(a) * (r - l), cx + cosf(a) * r, cy + sinf(a) * r, i % 5 == 0 ? 3 : 1.5f, SCHWARZ);
    }
}
static void zeiger(int cx, int cy, float a, int l, float d) {
    display.drawWideLine(cx, cy, cx + cosf(a) * l, cy + sinf(a) * l, d, SCHWARZ);
    display.fillCircle(cx, cy, 7, SCHWARZ);
}
static void rahmen(int cx, int cy, const char* name) {
    for (int i = 0; i < 3; i++) display.drawCircle(cx, cy, R - i, SCHWARZ);
    text(name, cx, cy - R - 16, &FreeSans9pt7b, MC_DATUM);
}

static void klima(int cx) {
    float t, rh;
    bool ok = klimaLesen(t, rh);
    rahmen(cx, CY, "Klima");
    const float a0 = 135 * DEG_TO_RAD, a1 = 405 * DEG_TO_RAD;   // 0…40 °C über 270°
    skala(cx, CY, R - 8, 41, a0, a1);
    char b[32];
    if (ok) {
        zeiger(cx, CY, a0 + (a1 - a0) * constrain(t, 0.0f, 40.0f) / 40, R - 22, 4);
        snprintf(b, sizeof b, "%.1f °C", t); text(b, cx, CY + R + 22, &FreeSansBold12pt7b, MC_DATUM);
        snprintf(b, sizeof b, "%.0f %%  ·  Tau %.1f °C", rh, taupunkt(t, rh)); text(b, cx, CY + R + 50, &FreeSans9pt7b, MC_DATUM);
        if (t - taupunkt(t, rh) < 2.5f) text("beschlägt gleich!", cx, CY + R + 74, &FreeSansBold9pt7b, MC_DATUM);
        if (millis() - zuletztCsv > 60000 || !zuletztCsv) {
            zuletztCsv = millis();
            snprintf(b, sizeof b, "%.0f%% tau %.1f", rh, taupunkt(t, rh));
            messung("instrumente", "klima", lroundf(t * 10), "0.1C", b);
        }
    } else text("kein SHT40", cx, CY + R + 22, &FreeSans9pt7b, MC_DATUM);
}

static void waage(int cx) {
    float ax, ay, az;
    bool ok = lageLesen(ax, ay, az);
    rahmen(cx, CY, "Wasserwaage");
    display.drawCircle(cx, CY, 20, SCHWARZ);
    display.drawFastHLine(cx - R + 10, CY, 2 * R - 20, SCHWARZ);
    display.drawFastVLine(cx, CY - R + 10, 2 * R - 20, SCHWARZ);
    if (!ok) { text("kein LSM6DS3", cx, CY + R + 22, &FreeSans9pt7b, MC_DATUM); return; }
    // Blase: wandert gegen die Neigung, 1 g = Rand
    int bx = cx + constrain(-ax, -1.0f, 1.0f) * (R - 22), by = CY + constrain(ay, -1.0f, 1.0f) * (R - 22);
    display.fillCircle(bx, by, 15, SCHWARZ);
    display.fillCircle(bx - 5, by - 5, 4, WEISS);
    float kippX = atan2f(ax, az) * RAD_TO_DEG, kippY = atan2f(ay, az) * RAD_TO_DEG;
    char b[40];
    snprintf(b, sizeof b, "%+.0f° / %+.0f°", kippX, kippY);
    text(b, cx, CY + R + 22, &FreeSansBold12pt7b, MC_DATUM);
    snprintf(b, sizeof b, "x %.2f  y %.2f  z %.2f g", ax, ay, az);
    text(b, cx, CY + R + 50, &FreeSans9pt7b, MC_DATUM);
}

static void uhr(int cx) {
    Uhrzeit u;
    bool ok = uhrLesen(u);
    rahmen(cx, CY, "Uhr");
    for (int i = 0; i < 12; i++) {   // Stundenstriche, Braun-ABW-artig: schlicht
        float a = i * 30 * DEG_TO_RAD - HALF_PI;
        int l = i % 3 == 0 ? 14 : 8;
        display.drawWideLine(cx + cosf(a) * (R - 10 - l), CY + sinf(a) * (R - 10 - l), cx + cosf(a) * (R - 10), CY + sinf(a) * (R - 10), i % 3 == 0 ? 4 : 2, SCHWARZ);
    }
    if (!ok) { text("kein PCF8563", cx, CY + R + 22, &FreeSans9pt7b, MC_DATUM); return; }
    float am = (u.m + u.s / 60.0f) * 6 * DEG_TO_RAD - HALF_PI;
    float ah = ((u.h % 12) + u.m / 60.0f) * 30 * DEG_TO_RAD - HALF_PI;
    zeiger(cx, CY, ah, R - 40, 7);
    zeiger(cx, CY, am, R - 18, 4);
    char b[32];
    snprintf(b, sizeof b, "%02d:%02d", u.h, u.m); text(b, cx, CY + R + 22, &FreeSansBold12pt7b, MC_DATUM);
    snprintf(b, sizeof b, "%02d.%02d.%04d%s", u.tag, u.monat, u.jahr, u.gueltig ? "" : " (?)");
    text(b, cx, CY + R + 50, &FreeSans9pt7b, MC_DATUM);
    if (!u.gueltig) text("Uhr war ohne Strom", cx, CY + R + 74, &FreeSans9pt7b, MC_DATUM);
}

static void akku(int cx) {
    AkkuStand a = akkuLesen();
    rahmen(cx, CY, "Akku");
    // Spannungsskala 3,3 … 4,2 V über 270°
    const float a0 = 135 * DEG_TO_RAD, a1 = 405 * DEG_TO_RAD;
    skala(cx, CY, R - 8, 19, a0, a1);
    float v = a.mv / 1000.0f;
    if (a.mv > 0) zeiger(cx, CY, a0 + (a1 - a0) * constrain((v - 3.3f) / 0.9f, 0.0f, 1.0f), R - 22, 4);
    char b[40];
    snprintf(b, sizeof b, "%.2f V", v); text(b, cx, CY + R + 22, &FreeSansBold12pt7b, MC_DATUM);
    snprintf(b, sizeof b, "%d mA  ·  %s", a.ma, usbDa() ? (laedt() ? "lädt" : "USB") : "Akku");
    text(b, cx, CY + R + 50, &FreeSans9pt7b, MC_DATUM);
    text("3,3", cx - 34, CY + 66, &FreeSans9pt7b, MC_DATUM);
    text("4,2", cx + 34, CY + 66, &FreeSans9pt7b, MC_DATUM);
}

static void alle() {
    display.fillRect(0, IY - 20, W, IH + 70, WEISS);
    uint32_t t0 = micros();
    klima(CX[0]); waage(CX[1]); uhr(CX[2]); akku(CX[3]);
    zuletzt = millis();
    Serial.printf("Instrumente: %lu µs\n", (unsigned long)(micros() - t0));
}
static void betreten() {
    statuszeile("7 · Instrumente");
    alle();
    text("alle 10 s neu  ·  Tippen = jetzt  ·  Klima jede Minute in die CSV", W / 2, 462, &FreeSans9pt7b, MC_DATUM);
}
static void tick() {
    if (millis() - zuletzt < 10000) return;
    alle();
    teil(0, IY - 20, W, IH + 70);
}
static void tipp(int, int) { alle(); teil(0, IY - 20, W, IH + 70); }
static void taste(Knopf) { tipp(0, 0); }

}  // namespace instrumente
