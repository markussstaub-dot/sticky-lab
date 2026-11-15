// Akku: Füllstandsmesser BQ27220 (0x55) am MISC-I²C, Strom-Sammler für Energie-Messungen
// Register laut TI-Datenblatt (Standard Commands, je 16 bit little endian). EDC fand 0x0C beim Laden unplausibel
// (mehrere A) – das Lab zeigt deshalb auch die Rohwerte.
#pragma once
#include <Wire.h>

static SemaphoreHandle_t wireSperre = nullptr;

enum : uint8_t {
    BQ_TEMP = 0x06, BQ_SPANNUNG = 0x08, BQ_STATUS = 0x0A, BQ_STROM = 0x0C, BQ_REST = 0x10, BQ_VOLL = 0x12,
    BQ_STROM_MITTEL = 0x14, BQ_ZYKLEN = 0x2A, BQ_SOC = 0x2C, BQ_SOH = 0x2E, BQ_DESIGN = 0x3C,
};

static bool bqLesen(uint8_t reg, int16_t& v) {
    if (wireSperre) xSemaphoreTake(wireSperre, portMAX_DELAY);
    bool ok = false;
    Wire.beginTransmission(0x55);
    Wire.write(reg);
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x55, 2) == 2) {
        v = (int16_t)(Wire.read() | Wire.read() << 8);
        ok = true;
    }
    if (wireSperre) xSemaphoreGive(wireSperre);
    return ok;
}
static int bqWert(uint8_t reg, int ersatz = -9999) { int16_t v; return bqLesen(reg, v) ? v : ersatz; }

static int akkuProzent() { int v = bqWert(BQ_SOC, -1); return v >= 0 && v <= 100 ? v : -1; }
static bool usbDa() { return analogReadMilliVolts(PIN_USB) > 1500; }
static bool laedt() { return digitalRead(PIN_CHG_STATE) == LOW; }

struct AkkuStand { int mv, ma, maMittel, rest, voll, design, soc, soh, zyklen; float grad; };
static AkkuStand akkuLesen() {
    AkkuStand a;
    a.mv = bqWert(BQ_SPANNUNG); a.ma = bqWert(BQ_STROM); a.maMittel = bqWert(BQ_STROM_MITTEL);
    a.rest = bqWert(BQ_REST); a.voll = bqWert(BQ_VOLL); a.design = bqWert(BQ_DESIGN);
    a.soc = bqWert(BQ_SOC); a.soh = bqWert(BQ_SOH); a.zyklen = bqWert(BQ_ZYKLEN);
    int t = bqWert(BQ_TEMP);
    a.grad = t == -9999 ? NAN : t / 10.0f - 273.15f;
    return a;
}
static void akkuSeriell() {
    AkkuStand a = akkuLesen();
    Serial.printf("AKKU %d %d %d %d %d %d %d %d %d %.1f %d %d\n", a.mv, a.ma, a.maMittel, a.rest, a.voll, a.design, a.soc,
                  a.soh, a.zyklen, a.grad, usbDa() ? 1 : 0, laedt() ? 1 : 0);
}

// ---- Strom-Sammler: eigene Aufgabe liest alle 250 ms den Strom und integriert ----------------------
// Der BQ27220 rechnet den Strom einmal je Sekunde neu; häufiger lesen schadet nicht, das Integral bleibt richtig.
static volatile bool sammeln = false;
static volatile double sammelMas = 0;      // mA·s (Entladung negativ, wie der Chip meldet)
static volatile uint32_t sammelProben = 0;
static volatile int sammelMin = 0, sammelMax = 0;

static void sammlerAufgabe(void*) {
    uint32_t zuletzt = millis();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(250));
        uint32_t jetzt = millis();
        if (sammeln) {
            int16_t ma;
            if (bqLesen(BQ_STROM, ma)) {
                sammelMas = sammelMas + ma * (jetzt - zuletzt) / 1000.0;
                if (!sammelProben || ma < sammelMin) sammelMin = ma;
                if (!sammelProben || ma > sammelMax) sammelMax = ma;
                sammelProben = sammelProben + 1;
            }
        }
        zuletzt = jetzt;
    }
}
static void akkuInit() {
    wireSperre = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(sammlerAufgabe, "sammler", 3072, nullptr, 1, nullptr, 0);
}
static void sammelnStart() { sammelMas = 0; sammelProben = 0; sammelMin = sammelMax = 0; sammeln = true; }
static void sammelnStopp() { sammeln = false; }
