/**
 * Sticky Lab – Werkbank für e-Ink-Technik und Design auf dem reTerminal Sticky (ohne App, ohne BLE)
 * Spec: ../SPEC.md · Nachschlagewerk: reterminal-sticky (EDC, FW 0.31.8)
 *
 * Bedienung: KI kurz = Übersicht · KI halten = Bildschirmfoto nach /lab/fotos · ▲/▼ = blättern · Tippen = Aktion
 * USB-Seriell (115200): „f“ = Foto über USB (werkzeuge/foto2png.py), „1“…„7“ = Station, „0“ = Übersicht
 *
 * Bauen: arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc"
 *        --build-property build.flash_size=32MB --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
 */
#define LAB_VERSION "0.5.3"

#include <esp_sleep.h>
#include <driver/gpio.h>
#include "pins.h"
#include "anzeige.h"
#include "akku.h"
#include "eingabe.h"
#include "karte.h"
#include "foto.h"
#include "station.h"
#include "stationen/uebersicht.h"
#include "stationen/refresh.h"
#include "stationen/energie.h"
#include "stationen/touch.h"
#include "stationen/braunkasten.h"
#include "stationen/rasterlabor.h"
#include "stationen/instrumente.h"
#include "stationen/schriften.h"
#include "stationen/bewegung.h"
#include "stationen/burgduell.h"

// ---- Stationen (Index 0 = Übersicht, danach Nummer wie in der SPEC) ------------------------
static const Station STATIONEN[] = {
    { "Übersicht", uebersichtBetreten, uebersichtTaste, uebersichtTipp, nullptr },
    { "1 · Refresh-Labor", refresh::betreten, refresh::taste, refresh::tipp, nullptr },
    { "2 · Schrift", schriften::betreten, schriften::taste, schriften::tipp, nullptr },
    { "3 · Raster", rasterlabor::betreten, rasterlabor::taste, rasterlabor::tipp, nullptr },
    { "4 · Braun-Baukasten", braunkasten::betreten, braunkasten::taste, braunkasten::tipp, nullptr },
    { "5 · Bewegung", bewegung::betreten, bewegung::taste, bewegung::tipp, nullptr },
    { "6 · Touch", touch::betreten, touch::taste, touch::tipp, touch::kritzelnTick },
    { "7 · Instrumente", instrumente::betreten, instrumente::taste, instrumente::tipp, instrumente::tick },
    { "8 · Energie", energie::betreten, energie::taste, energie::tipp, nullptr },
    { "9 · Burgduell", burgduell::betreten, burgduell::taste, burgduell::tipp, nullptr },
};
static const int ANZAHL = sizeof STATIONEN / sizeof STATIONEN[0];
static int aktuell = -1;

static void geheZu(int i) {
    if (i < 0 || i >= ANZAHL || !STATIONEN[i].betreten) return;
    aktuell = i;
    tpPunkt = nullptr;   // nur die Station, die jeden Touch-Punkt braucht, meldet sich wieder an
    display.fillScreen(WEISS);
    if (i == 0) { char t[48]; snprintf(t, sizeof t, "Übersicht  ·  FW " LAB_VERSION "  ·  %s", panelChip()); statuszeile(t); }
    STATIONEN[i].betreten();
    voll();
    Serial.printf("STATION %d %s\n", i, STATIONEN[i].name);
}

// ---- KI-Taste: kurz = Übersicht, halten = Foto -------------------------------------------------
static uint32_t kiSeit = 0;
static bool kiFotoGemacht = false;

static void fotoMachen() {
    kiFotoGemacht = true;
    int n = fotoAufKarte();
    if (n > 0) { tone(PIN_BUZZER, 2200, 40); delay(80); tone(PIN_BUZZER, 3000, 40); }
    else tone(PIN_BUZZER, 600, 200);
}

static void tasteCb(Knopf k, bool unten) {
    if (!usbDa()) messung("diag", "taste", k * 10 + (unten ? 1 : 0), "k*10+unten");   // Fehlersuche Akkubetrieb
    if (k == K_KI) {
        if (unten) { kiSeit = millis(); kiFotoGemacht = false; }
        else if (!kiFotoGemacht) geheZu(0);
        return;
    }
    if (unten && aktuell >= 0 && STATIONEN[aktuell].taste) STATIONEN[aktuell].taste(k);
}
static void touchCb(int x, int y, bool unten) {
    if (unten && aktuell >= 0 && STATIONEN[aktuell].tipp) STATIONEN[aktuell].tipp(x, y);
}

// ---- Schlaf: nach 3 min ohne Bedienung im Akkubetrieb leicht schlafen, Tasten wecken ------------
static const uint32_t SCHLAF_NACH_MS = 3 * 60 * 1000;
static void vielleichtSchlafen() {
    if (millis() - letzteBedienung < SCHLAF_NACH_MS || usbDa()) return;
    // Schlafbild muss ohne Strom stimmen: „Zzz“ in die Statuszeile, damit niemand das Gerät für tot hält
    display.fillRect(W - 300, 0, 300, STATUS_H - 2, WEISS);
    text("Zzz  ·  Taste weckt", W - 20, 23, &FreeSansBold12pt7b, MR_DATUM);
    teil(W - 300, 0, 300, STATUS_H - 2);
    messung("system", "schlaf", (millis() - letzteBedienung) / 1000, "s");
    uint32_t t0 = millis();
    Serial.println("Schlaf");
    Serial.flush();
    for (auto& t : tasten) gpio_wakeup_enable((gpio_num_t)t.pin, t.ruhe == HIGH ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    gpio_hold_en((gpio_num_t)PIN_PWR_HOLD);
    gpio_hold_en((gpio_num_t)PIN_PWR_LOCK);
    gpio_hold_en((gpio_num_t)PIN_CHG_EN_N);
    esp_light_sleep_start();
    gpio_hold_dis((gpio_num_t)PIN_CHG_EN_N);
    gpio_hold_dis((gpio_num_t)PIN_PWR_HOLD);
    gpio_hold_dis((gpio_num_t)PIN_PWR_LOCK);
    for (auto& t : tasten) gpio_wakeup_disable((gpio_num_t)t.pin);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // die weckende Taste nicht als Bedienung auswerten
    delay(50);
    for (auto& t : tasten) { t.stand = digitalRead(t.pin); t.gedrueckt = false; t.seit = millis(); }
    bedient();
    klick();   // hörbar wach, auch wenn die weckende Taste sonst nichts tut
    messung("system", "wach", (millis() - t0) / 1000, "s");
    // Statuszeile zurück (ohne „Zzz“)
    if (aktuell >= 0) {
        display.fillRect(W - 300, 0, 300, STATUS_H - 2, WEISS);
        char r[24]; int p = akkuProzent();
        if (p >= 0) snprintf(r, sizeof r, "%d %%  ·  LAB", p); else snprintf(r, sizeof r, "LAB");
        text(r, W - 20, 23, &FreeSans12pt7b, MR_DATUM);
        teil(W - 300, 0, 300, STATUS_H - 2);
    }
    Serial.println("wach");
}

// ---- Ablauf ---------------------------------------------------------------------------------
void setup() {
    setCpuFrequencyMhz(80);   // wie EDC, damit Messungen vergleichbar sind
    pinMode(PIN_PWR_HOLD, OUTPUT);
    digitalWrite(PIN_PWR_HOLD, HIGH);
    pinMode(PIN_PWR_LOCK, OUTPUT);
    digitalWrite(PIN_PWR_LOCK, HIGH);   // wie Seeed: HOLD und LOCK zuerst
    pinMode(PIN_CHG_EN_N, OUTPUT);
    digitalWrite(PIN_CHG_EN_N, LOW);   // Laden erlauben
    pinMode(PIN_CHG_STATE, INPUT_PULLUP);
    pinMode(PIN_USB, INPUT);
    analogSetPinAttenuation(PIN_USB, ADC_11db);
    Serial.begin(115200);
    delay(300);
    Serial.printf("\nSticky Lab %s\n", LAB_VERSION);
    Wire.begin(PIN_MISC_SDA, PIN_MISC_SCL, 100000);
    randomSeed(esp_random());   // echte Zufallszahlen (Burgduell-Gelände, Wind)
    akkuInit();

    if (!display.begin()) Serial.println(display.lastResult().message);
    // erst nach display.begin()
    sdStart();
    tastenInit();
    touchInit();
    bedient();
    messung("system", "start", (int)esp_reset_reason(), "grund", usbDa() ? "usb" : "akku");
    {   // welcher Display-Controller steckt in diesem Gerät?
        char p[48];
        snprintf(p, sizeof p, "%s grau4 %s", panelChip(), panelGrau4() ? "ja" : "nein");
        messung("system", "panel", 0, "-", p);
    }
    {   // Fehlersuche: gelernte Ruhepegel und aktuelle Pegel der Tasten
        char p[48];
        snprintf(p, sizeof p, "ruhe %d%d%d jetzt %d%d%d", tasten[0].ruhe, tasten[1].ruhe, tasten[2].ruhe,
                 digitalRead(PIN_BTN_KI), digitalRead(PIN_BTN_UP), digitalRead(PIN_BTN_DOWN));
        messung("diag", "tasten", 0, "-", p);
    }
    geheZu(0);
}

void loop() {
    {   // Fehlersuche: im Akkubetrieb in den ersten 2 Minuten alle 5 s ein Lebenszeichen auf die Karte
        static uint32_t runden = 0, zuletzt = 0;
        runden++;
        if (!usbDa() && millis() < 120000 && millis() - zuletzt > 5000) {
            zuletzt = millis();
            char p[48];
            snprintf(p, sizeof p, "pegel %d%d%d rx %d", digitalRead(PIN_BTN_KI), digitalRead(PIN_BTN_UP), digitalRead(PIN_BTN_DOWN), Serial.available());
            messung("diag", "lebt", runden, "runden", p);
        }
    }
    tastenPoll(tasteCb);
    if (tasteUnten(K_KI) && !kiFotoGemacht && millis() - kiSeit > 800) fotoMachen();
    touchPoll(touchCb);   // holt nur ab, was der Touch-Sammler gesammelt hat
    if (aktuell >= 0 && STATIONEN[aktuell].tick) STATIONEN[aktuell].tick();
    // USB-Befehle (Sticky-Console, console/index.html): je ein Zeichen, „t x y“ mit Zeilenende
    // Ohne USB ist der CH343 stromlos und seine Leitung zum ESP schwebt: Rauschen wäre sonst ein Befehl (z. B. „f“ =
    // 4 s Foto senden). Darum nur am USB auswerten, sonst verwerfen und zählen.
    static uint32_t rauschen = 0;
    if (!usbDa()) {
        while (Serial.available()) { Serial.read(); rauschen++; }
        static uint32_t gemeldet = 0, gemeldetUm = 0;
        if (rauschen != gemeldet && millis() - gemeldetUm > 60000) {   // höchstens einmal je Minute auf die Karte
            messung("system", "uart-rauschen", rauschen, "byte"); gemeldet = rauschen; gemeldetUm = millis();
        }
    }
    while (usbDa() && Serial.available()) {
        int c = Serial.read();
        if (c == 'f') fotoSeriell();
        else if (c >= '0' && c <= '9') geheZu(c - '0');
        else if (c == 'u') { bedient(); tasteCb(K_UP, true); }
        else if (c == 'd') { bedient(); tasteCb(K_DOWN, true); }
        else if (c == 'k') { bedient(); kiFotoGemacht = false; tasteCb(K_KI, false); }   // KI kurz
        else if (c == 'p') fotoMachen();                                                  // Foto auf die Karte
        else if (c == 't') {
            int x = Serial.parseInt(), y = Serial.parseInt();
            bedient();
            touchCb(x, y, true);
        } else if (c == 'a') akkuSeriell();
        else if (c == 'c') {   // Messungen von der Karte über USB
            File f = sdEin() ? SD.open("/lab/messungen.csv") : File();
            Serial.println("CSV-ANFANG");
            while (f && f.available()) Serial.write(f.read());
            if (f) f.close();
            Serial.println("CSV-ENDE");
        }
        else if (c == 'i') {
            Serial.printf("INFO %s %d %d %d %s %d %d %d %.2f %s %d\n", LAB_VERSION, aktuell, akkuProzent(), usbDa() ? 1 : 0, sdDa ? "sd" : "-", PANEL.w, PANEL.h, PANEL.farben, PANEL.pxProMm, panelChip(), panelGrau4() ? 1 : 0);
        }
    }
    vielleichtSchlafen();
    delay(5);
}
