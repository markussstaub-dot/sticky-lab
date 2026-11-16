/**
 * Sticky Lab – Werkbank für e-Ink-Technik und Design auf dem reTerminal Sticky (ohne App, ohne BLE)
 * Spec: ../SPEC.md · Nachschlagewerk: reterminal-sticky (EDC, FW 0.31.8)
 *
 * Bedienung: KI kurz = Übersicht · KI halten = Bildschirmfoto nach /lab/fotos · ▲/▼ = blättern · Tippen = Aktion
 * USB-Seriell (115200): „f“ = Foto über USB (werkzeuge/foto2png.py), „s1“…„s11“ = Station, „s0“ = Übersicht,
 *   „t x y“ = Tipp, „z/m x y“ + „h“ = Strich, „i“ Info, „a“ Akku, „c“ Messungen
 *
 * Bauen: arduino-cli compile -b "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,CDCOnBoot=cdc"
 *        --build-property build.flash_size=32MB --build-path "%LOCALAPPDATA%\sticky-build\sticky-lab" firmware
 */
#define LAB_VERSION "0.9.1"
#define PLATZ_EIGEN 1   // Lab gehört nach app1 (platz.h); aus dem Playground läuft es in app0

#include <esp_sleep.h>
#include <driver/gpio.h>
#include "pins.h"
#include "anzeige.h"
#include "akku.h"
#include "eingabe.h"
#include "karte.h"
#include "foto.h"
#include "platz.h"
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
#include "stationen/tastatur.h"
#include "stationen/pins.h"
#include "hilfe.h"

// ---- Stationen (Index 0 = Übersicht, danach Nummer wie in der SPEC) ------------------------
static const Station STATIONEN[] = {
    { "Übersicht", uebersichtBetreten, uebersichtTaste, uebersichtTipp, nullptr },
    { "1 · Refresh-Labor", refresh::betreten, refresh::taste, refresh::tipp, nullptr },
    { "2 · Schrift", schriften::betreten, schriften::taste, schriften::tipp, nullptr },
    { "3 · Raster", rasterlabor::betreten, rasterlabor::taste, rasterlabor::tipp, nullptr },
    { "4 · Braun-Baukasten", braunkasten::betreten, braunkasten::taste, braunkasten::tipp, braunkasten::tick },
    { "5 · Bewegung", bewegung::betreten, bewegung::taste, bewegung::tipp, nullptr },
    { "6 · Touch", touch::betreten, touch::taste, touch::tipp, touch::kritzelnTick },
    { "7 · Instrumente", instrumente::betreten, instrumente::taste, instrumente::tipp, instrumente::tick },
    { "8 · Energie", energie::betreten, energie::taste, energie::tipp, nullptr },
    { "9 · Burgduell", burgduell::betreten, burgduell::taste, burgduell::tipp, nullptr },
    { "10 · Tastatur", tastatur::betreten, tastatur::taste, tastatur::tipp, tastatur::tick },
    { "11 · Pins & Neustarts", pinstation::betreten, pinstation::taste, pinstation::tipp, pinstation::tick },
};
static const int ANZAHL = sizeof STATIONEN / sizeof STATIONEN[0];
static int aktuell = -1;
static const int STATION_PINS = 11;   // zählt zusätzliche Pins nur, solange sie offen ist

static void geheZu(int i) {
    if (i < 0 || i >= ANZAHL || !STATIONEN[i].betreten) return;
    if (aktuell == STATION_PINS && i != STATION_PINS) pinstation::verlassen();
    aktuell = i;
    tpPunkt = nullptr;   // nur die Station, die jeden Touch-Punkt braucht, meldet sich wieder an
    display.fillScreen(WEISS);
    if (i == 0) { char t[48]; snprintf(t, sizeof t, T("Übersicht  ·  FW " LAB_VERSION "  ·  %s", "Overview  ·  FW " LAB_VERSION "  ·  %s"), panelChip()); statuszeile(t); }
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

// ▼ in der Übersicht: kurz = Station 1, 2 s halten = Burgduell (Geheimnis ohne Kachel, Markus 16.11.2026)
static uint32_t runterSeit = 0;
static bool runterVerbraucht = true;
static const uint32_t GEHEIM_MS = 2000;

static void tasteCb(Knopf k, bool unten) {
    if (k == K_KI) {
        if (unten) { kiSeit = millis(); kiFotoGemacht = false; }
        else if (!kiFotoGemacht) geheZu(0);
        return;
    }
    if (hilfeAn) { if (unten) { hilfeAn = false; geheZu(aktuell); } return; }   // jede Taste schließt die Hilfe
    // ▲ und ▼ zusammen = Hilfe (wie EDC)
    if (unten && ((k == K_UP && tasteUnten(K_DOWN)) || (k == K_DOWN && tasteUnten(K_UP)))) { runterVerbraucht = true; tpPunkt = nullptr; hilfeZeigen(aktuell); return; }
    if (aktuell == 0 && k == K_DOWN) {
        if (unten) { runterSeit = millis(); runterVerbraucht = false; }
        else if (!runterVerbraucht) { runterVerbraucht = true; geheZu(1); }
        return;
    }
    if (unten && aktuell >= 0 && STATIONEN[aktuell].taste) STATIONEN[aktuell].taste(k);
}
static void touchCb(int x, int y, bool unten) {
    if (!unten) return;
    if (hilfeAn) { hilfeAn = false; geheZu(aktuell); return; }                // Tipp schließt die Hilfe
    if (y < STATUS_H) { tpPunkt = nullptr; hilfeZeigen(aktuell); return; }   // Tipp auf die Statuszeile = Hilfe
    if (aktuell >= 0 && STATIONEN[aktuell].tipp) STATIONEN[aktuell].tipp(x, y);
}

// ---- Schlaf: nach 3 min ohne Bedienung im Akkubetrieb leicht schlafen, Tasten wecken ------------
static const uint32_t SCHLAF_NACH_MS = 3 * 60 * 1000;
static void vielleichtSchlafen() {
    if (millis() - letzteBedienung < SCHLAF_NACH_MS || usbDa()) return;
    // Schlafbild muss ohne Strom stimmen: „Zzz“ in die Statuszeile, damit niemand das Gerät für tot hält
    display.fillRect(W - 300, 0, 300, STATUS_H - 2, WEISS);
    text(T("Zzz  ·  Taste weckt", "Zzz  ·  press a key"), W - 20, 23, &FreeSansBold12pt7b, MR_DATUM);
    teil(W - 300, 0, 300, STATUS_H - 2);
    messung("system", "schlaf", (millis() - letzteBedienung) / 1000, "s");
    uint32_t t0 = millis();
    Serial.println("Schlaf");
    Serial.flush();
    tastenSchlafen();
    esp_sleep_enable_gpio_wakeup();
    gpio_hold_en((gpio_num_t)PIN_PWR_HOLD);
    gpio_hold_en((gpio_num_t)PIN_PWR_LOCK);
    gpio_hold_en((gpio_num_t)PIN_CHG_EN_N);
    esp_light_sleep_start();
    gpio_hold_dis((gpio_num_t)PIN_CHG_EN_N);
    gpio_hold_dis((gpio_num_t)PIN_PWR_HOLD);
    gpio_hold_dis((gpio_num_t)PIN_PWR_LOCK);
    tastenWach();
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
        char r[32]; int p = akkuProzent();
        if (p >= 0) snprintf(r, sizeof r, "%d %%  ·  LAB  ·  ?", p); else snprintf(r, sizeof r, "LAB  ·  ?");
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
    size_t rxPuffer = Serial.setRxBufferSize(8192);   // Striche aus der Console kommen auch während eines Refreshs (~0,9 s) an
    Serial.begin(115200);
    Serial.setTimeout(50);   // parseInt wartet sonst bis 1 s je Zahl
    delay(300);
    Serial.printf("\nSticky Lab %s, RX-Puffer %u\n", LAB_VERSION, (unsigned)rxPuffer);
    platzNotausgang(PIN_BTN_UP);   // ▲ beim Einschalten halten = anderer Firmware-Platz (EDC)
    Wire.begin(PIN_MISC_SDA, PIN_MISC_SCL, 100000);
    spracheLaden();
    randomSeed(esp_random());   // echte Zufallszahlen (Burgduell-Gelände, Wind)
    akkuInit();

    if (!display.begin()) Serial.println(display.lastResult().message);
    // erst nach display.begin()
    sdStart();
    pinstation::start();   // Neustart zählen und Grund merken, vor den Interrupts (Station 11)
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
    // Absturzschleife (3x in Folge Absturz/Watchdog): gleich die Station zeigen, die sie erklärt
    geheZu(pinstation::folge >= 3 ? STATION_PINS : 0);
}

void loop() {
    diagPoll();
    tastenPoll(tasteCb);
    if (tasteUnten(K_KI) && !kiFotoGemacht && millis() - kiSeit > 800) fotoMachen();
    if (aktuell == 0 && !hilfeAn && !runterVerbraucht && tasteUnten(K_DOWN) && millis() - runterSeit > GEHEIM_MS) {
        runterVerbraucht = true;
        tone(PIN_BUZZER, 1800, 60); delay(90); tone(PIN_BUZZER, 2400, 60); delay(90); tone(PIN_BUZZER, 3200, 90);
        geheZu(9);
    }
    touchPoll(touchCb);   // holt nur ab, was der Touch-Sammler gesammelt hat
    if (!hilfeAn && aktuell >= 0 && STATIONEN[aktuell].tick) STATIONEN[aktuell].tick();
    static bool strichEcho = false;   // Fehlersuche: empfangene Strich-Punkte zurückmelden (bei Bedarf true)
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
        else if (c == 's') geheZu(Serial.parseInt());   // „s6“ = Station 6; lose Ziffern (Reste bei Rückstau) zählen nicht
        else if (c == 'u') { bedient(); tasteCb(K_UP, true); }
        else if (c == 'd') { bedient(); tasteCb(K_DOWN, true); tasteCb(K_DOWN, false); }   // kurz (Übersicht wertet beim Loslassen aus)
        else if (c == 'k') { bedient(); kiFotoGemacht = false; tasteCb(K_KI, false); }   // KI kurz
        else if (c == 'p') fotoMachen();                                                  // Foto auf die Karte
        else if (c == 't') {
            int x = Serial.parseInt(), y = Serial.parseInt();
            bedient();
            touchCb(x, y, true);
        } else if (c == 'z' || c == 'm' || c == 'h') {
            // Striche aus der Console (Maus, Finger oder S Pen am Fold): wie echter Touch in die Warteschlange
            static int16_t fx = 0, fy = 0;
            TpEreignis e;
            // Achtung: constrain() ist ein Makro und wertet sein Argument mehrfach aus – erst lesen, dann begrenzen
            if (c != 'h') { int x = Serial.parseInt(), y = Serial.parseInt(); fx = constrain(x, 0, W - 1); fy = constrain(y, 0, H - 1); }
            e.x = fx; e.y = fy; e.art = c == 'z' ? 0 : c == 'm' ? 1 : 2;
            if (tpSchlange) xQueueSend(tpSchlange, &e, 0);
            if (strichEcho) Serial.printf("P %c %d %d %d\n", c, fx, fy, (int)analogReadMilliVolts(PIN_USB));
        } else if (c == 'a') akkuSeriell();
        else if (c == 'l') { EN = !EN; spracheMerken(); geheZu(aktuell); }   // Sprache umschalten (DE/EN)
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
