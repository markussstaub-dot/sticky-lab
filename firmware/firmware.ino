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
#define LAB_VERSION "0.2.0"

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

// ---- Stationen (Index 0 = Übersicht, danach Nummer wie in der SPEC) ------------------------
static const Station STATIONEN[] = {
    { "Übersicht", uebersichtBetreten, uebersichtTaste, uebersichtTipp, nullptr },
    { "1 · Refresh-Labor", refresh::betreten, refresh::taste, refresh::tipp, nullptr },
    {}, {}, {}, {}, {}, {},   // 2–7 kommen noch
    { "8 · Energie", energie::betreten, energie::taste, energie::tipp, nullptr },
};
static const int ANZAHL = sizeof STATIONEN / sizeof STATIONEN[0];
static int aktuell = -1;

static void geheZu(int i) {
    if (i < 0 || i >= ANZAHL || !STATIONEN[i].betreten) return;
    aktuell = i;
    display.fillScreen(WEISS);
    if (i == 0) statuszeile("Übersicht  ·  FW " LAB_VERSION);
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
    Serial.println("Schlaf");
    Serial.flush();
    for (auto& t : tasten) gpio_wakeup_enable((gpio_num_t)t.pin, t.ruhe == HIGH ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    gpio_hold_en((gpio_num_t)PIN_PWR_HOLD);
    gpio_hold_en((gpio_num_t)PIN_CHG_EN_N);
    esp_light_sleep_start();
    gpio_hold_dis((gpio_num_t)PIN_CHG_EN_N);
    gpio_hold_dis((gpio_num_t)PIN_PWR_HOLD);
    for (auto& t : tasten) gpio_wakeup_disable((gpio_num_t)t.pin);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // die weckende Taste nicht als Bedienung auswerten
    delay(50);
    for (auto& t : tasten) { t.stand = digitalRead(t.pin); t.gedrueckt = false; t.seit = millis(); }
    bedient();
    Serial.println("wach");
}

// ---- Ablauf ---------------------------------------------------------------------------------
void setup() {
    setCpuFrequencyMhz(80);   // wie EDC, damit Messungen vergleichbar sind
    pinMode(PIN_PWR_HOLD, OUTPUT);
    digitalWrite(PIN_PWR_HOLD, HIGH);
    pinMode(PIN_CHG_EN_N, OUTPUT);
    digitalWrite(PIN_CHG_EN_N, LOW);   // Laden erlauben
    pinMode(PIN_CHG_STATE, INPUT_PULLUP);
    pinMode(PIN_USB, INPUT);
    analogSetPinAttenuation(PIN_USB, ADC_11db);
    Serial.begin(115200);
    delay(300);
    Serial.printf("\nSticky Lab %s\n", LAB_VERSION);
    Wire.begin(PIN_MISC_SDA, PIN_MISC_SCL, 100000);
    akkuInit();

    if (!display.begin()) Serial.println(display.lastResult().message);
    // erst nach display.begin()
    sdStart();
    tastenInit();
    touchInit();
    bedient();
    geheZu(0);
}

void loop() {
    tastenPoll(tasteCb);
    if (tasteUnten(K_KI) && !kiFotoGemacht && millis() - kiSeit > 800) fotoMachen();
    static uint32_t tp = 0;
    if (millis() - tp > 25) { tp = millis(); touchPoll(touchCb); }
    if (aktuell >= 0 && STATIONEN[aktuell].tick) STATIONEN[aktuell].tick();
    // USB-Befehle (Sticky-Console, console/index.html): je ein Zeichen, „t x y“ mit Zeilenende
    while (Serial.available()) {
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
        else if (c == 'i') {
            Serial.printf("INFO %s %d %d %d %s\n", LAB_VERSION, aktuell, akkuProzent(), usbDa() ? 1 : 0, sdDa ? "sd" : "-");
        }
    }
    vielleichtSchlafen();
    delay(5);
}
