// Sensoren am MISC-I²C (GPIO 0/1, mit der Sperre aus akku.h): Klima SHT40, Bewegung LSM6DS3TR-C, Uhr PCF8563
// Adressen laut Schaltplan (EDC-SPEC „Eigene Sensoren“).
#pragma once
#include <math.h>

static bool i2cSchreiben(uint8_t adr, const uint8_t* d, int n) {
    if (wireSperre) xSemaphoreTake(wireSperre, portMAX_DELAY);
    Wire.beginTransmission(adr);
    Wire.write(d, n);
    bool ok = Wire.endTransmission() == 0;
    if (wireSperre) xSemaphoreGive(wireSperre);
    return ok;
}
static bool i2cLesen(uint8_t adr, int reg, uint8_t* d, int n) {   // reg < 0: ohne Registeradresse lesen
    if (wireSperre) xSemaphoreTake(wireSperre, portMAX_DELAY);
    bool ok = true;
    if (reg >= 0) { Wire.beginTransmission(adr); Wire.write((uint8_t)reg); ok = Wire.endTransmission(false) == 0; }
    if (ok) ok = Wire.requestFrom((int)adr, n) == n;
    if (ok) for (int i = 0; i < n; i++) d[i] = Wire.read();
    if (wireSperre) xSemaphoreGive(wireSperre);
    return ok;
}

// ---- SHT40: Temperatur und Feuchte ---------------------------------------------------------------------
static bool klimaLesen(float& grad, float& feuchte) {
    uint8_t c = 0xFD, d[6];   // hohe Genauigkeit, ~9 ms
    if (!i2cSchreiben(0x44, &c, 1)) return false;
    delay(10);
    if (!i2cLesen(0x44, -1, d, 6)) return false;
    grad = -45 + 175.0f * (d[0] << 8 | d[1]) / 65535.0f;
    feuchte = constrain(-6 + 125.0f * (d[3] << 8 | d[4]) / 65535.0f, 0.0f, 100.0f);
    return true;
}
/** Taupunkt nach Magnus */
static float taupunkt(float t, float rh) {
    const float a = 17.62f, b = 243.12f;
    float g = logf(max(rh, 1.0f) / 100.0f) + a * t / (b + t);
    return b * g / (a - g);
}

// ---- LSM6DS3TR-C: Beschleunigung → Neigung -------------------------------------------------------------
static bool lageBereit = false;
static bool lageInit() {
    uint8_t id = 0;
    if (!i2cLesen(0x6A, 0x0F, &id, 1) || id != 0x6A) return false;
    uint8_t c[2] = { 0x10, 0x40 };   // CTRL1_XL: 104 Hz, ±2 g
    lageBereit = i2cSchreiben(0x6A, c, 2);
    delay(30);   // erste Messwerte abwarten (104 Hz)
    return lageBereit;
}
/** Beschleunigung in g (Achsen des Chips) */
static bool lageLesen(float& ax, float& ay, float& az) {
    if (!lageBereit && !lageInit()) return false;
    uint8_t d[6];
    if (!i2cLesen(0x6A, 0x28, d, 6)) return false;
    ax = (int16_t)(d[0] | d[1] << 8) * 0.000061f;
    ay = (int16_t)(d[2] | d[3] << 8) * 0.000061f;
    az = (int16_t)(d[4] | d[5] << 8) * 0.000061f;
    return true;
}

// ---- PCF8563: Uhr --------------------------------------------------------------------------------------
struct Uhrzeit { int s, m, h, tag, monat, jahr; bool gueltig; };
static int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static bool uhrLesen(Uhrzeit& u) {
    uint8_t d[7];
    if (!i2cLesen(0x51, 0x02, d, 7)) return false;
    u.gueltig = !(d[0] & 0x80);   // VL-Bit: Spannung war weg, Zeit nicht verlässlich
    u.s = bcd(d[0] & 0x7F); u.m = bcd(d[1] & 0x7F); u.h = bcd(d[2] & 0x3F);
    u.tag = bcd(d[3] & 0x3F); u.monat = bcd(d[5] & 0x1F); u.jahr = 2000 + bcd(d[6]);
    return true;
}
