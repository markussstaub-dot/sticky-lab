// Tasten und Touch – nach reterminal-sticky FW 0.31.8, ohne BLE
// Stolperfalle: tastenInit() und touchInit() erst NACH display.begin() aufrufen, sonst kommen keine Drücke an.
#pragma once
#include <Wire.h>

static uint32_t letzteBedienung = 0;
static void bedient() { letzteBedienung = millis(); }
static void klick() { tone(PIN_BUZZER, 2700, 30); }

// ---- Tasten ---------------------------------------------------------------------------
enum Knopf : uint8_t { K_KI = 0, K_UP, K_DOWN };
struct Taste { int pin; int ruhe; int stand; uint32_t seit; volatile bool gedrueckt; };
static Taste tasten[3] = { {PIN_BTN_KI}, {PIN_BTN_UP}, {PIN_BTN_DOWN} };
static void IRAM_ATTR tasteIsr(void* arg) { ((Taste*)arg)->gedrueckt = true; }

static void tastenInit() {
    for (auto& t : tasten) pinMode(t.pin, INPUT_PULLUP);
    delay(5);
    for (auto& t : tasten) {
        t.ruhe = digitalRead(t.pin);   // Ruhepegel lernen
        t.stand = t.ruhe;
        t.seit = millis();
        t.gedrueckt = false;
        attachInterruptArg(digitalPinToInterrupt(t.pin), tasteIsr, &t, t.ruhe == HIGH ? FALLING : RISING);
    }
}

/** Ruft cb(knopf, gedrückt) bei jeder Flanke. Drücke, die ganz in einen Refresh fielen, werden nachgeholt. */
static void tastenPoll(void (*cb)(Knopf, bool)) {
    for (int i = 0; i < 3; i++) {
        Taste& t = tasten[i];
        int v = digitalRead(t.pin);
        if (t.gedrueckt && v == t.ruhe && t.stand == t.ruhe) {
            t.gedrueckt = false;
            bedient(); klick();
            cb((Knopf)i, true); cb((Knopf)i, false);
            t.seit = millis();
            continue;
        }
        if (v == t.stand || millis() - t.seit < 30) continue;
        t.stand = v;
        t.seit = millis();
        t.gedrueckt = false;
        bool unten = v != t.ruhe;
        bedient();
        if (unten) klick();
        cb((Knopf)i, unten);
    }
}
static bool tasteUnten(Knopf k) { return tasten[k].stand != tasten[k].ruhe; }

// ---- Touch GT911 ----------------------------------------------------------------------------
TwoWire wireTp = TwoWire(1);
static uint8_t tpAddr = 0;
static bool tpWrite(uint16_t reg, uint8_t val) {
    wireTp.beginTransmission(tpAddr);
    wireTp.write(reg >> 8); wireTp.write(reg & 0xFF); wireTp.write(val);
    return wireTp.endTransmission() == 0;
}
static bool tpRead(uint16_t reg, uint8_t* buf, uint8_t n) {
    wireTp.beginTransmission(tpAddr);
    wireTp.write(reg >> 8); wireTp.write(reg & 0xFF);
    if (wireTp.endTransmission(false) != 0) return false;
    if (wireTp.requestFrom((int)tpAddr, (int)n) != n) return false;
    for (int i = 0; i < n; i++) buf[i] = wireTp.read();
    return true;
}

static void touchInit() {
    // Stromzyklus und Goodix-Reset wie in der Werks-Firmware (INT low beim Reset → Adresse 0x5D)
    pinMode(PIN_TP_RST, OUTPUT); pinMode(PIN_TP_INT, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW); digitalWrite(PIN_TP_INT, LOW);
    pinMode(PIN_TP_EN, OUTPUT);
    digitalWrite(PIN_TP_EN, LOW); delay(50);
    digitalWrite(PIN_TP_EN, HIGH); delay(30);
    digitalWrite(PIN_TP_RST, HIGH); delay(6);
    digitalWrite(PIN_TP_INT, LOW); delay(55);
    pinMode(PIN_TP_INT, INPUT); delay(100);
    wireTp.begin(PIN_TP_SDA, PIN_TP_SCL, 400000);
    for (uint8_t a : {0x5D, 0x14}) {
        wireTp.beginTransmission(a);
        if (wireTp.endTransmission() == 0) { tpAddr = a; break; }
    }
    if (!tpAddr) { Serial.println("Touch: kein GT911 gefunden"); return; }
    // Die Lab-Firmware schreibt keine Konfiguration; der Chip bringt eine mit (EDC schreibt nur, wenn sie fehlt).
    uint8_t ver = 0;
    tpRead(0x8047, &ver, 1);
    if (ver == 0) Serial.println("Touch: keine Konfiguration im Chip – einmal EDC-Firmware starten, die schreibt sie");
    tpWrite(0x8040, 0);   // normaler Lesemodus
    tpWrite(0x814E, 0);   // Statuspuffer leeren
    Serial.printf("Touch: GT911 an 0x%02X, Konfig 0x%02X\n", tpAddr, ver);
}

static bool tpUnten = false;
static int tpX = 0, tpY = 0;   // letzte Lage in Display-Koordinaten

/** cb(x, y, unten) bei Berühren und Loslassen; x/y im Querformat 800 × 480 */
static void touchPoll(void (*cb)(int, int, bool)) {
    if (!tpAddr) return;
    uint8_t st;
    if (!tpRead(0x814E, &st, 1) || !(st & 0x80)) return;
    uint8_t n = st & 0x0F;
    if (n > 0) {
        uint8_t p[7];
        tpRead(0x814F, p, 7);
        int rx = p[1] | p[2] << 8, ry = p[3] | p[4] << 8;
        tpX = 799 - ry; tpY = 479 - rx;   // GT911 liefert hochkant (am Gerät geprüft, EDC 11.11.2026)
        if (!tpUnten) { tpUnten = true; bedient(); cb(tpX, tpY, true); }
    } else if (tpUnten) {
        tpUnten = false;
        cb(tpX, tpY, false);
    }
    tpWrite(0x814E, 0);
}

static bool drin(int x, int y, int x0, int y0, int w, int h) { return x >= x0 && x < x0 + w && y >= y0 && y < y0 + h; }
