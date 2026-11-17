// c3_nachliefern – Fühler schreibt auf, solange niemand zuhört, und liefert nach, sobald die Basis antwortet
// (Station 12 „Fremdsensoren“, Versuch 2, 17.11.2026). Eine Firmware, zwei Rollen nach MAC:
//   BASIS  (MAC aus geraete.h, nicht im Repo; Vorlage geraete.h.vorlage) am Rechner: empfängt, bestätigt jeden Wert,
//          speichert ihn im Flash
//   FÜHLER (alle anderen, z. B. c3-CB30) im Auto an der Powerbank: jede Minute ein Messwert in den Flash, alle 10 s
//          alles Unbestätigte an die Basis (höchstens 20 je Runde). Bleibt WACH, sonst schaltet die Powerbank ab.
// Funk: ESP-NOW Broadcast auf Kanal 1, Funk schläft nicht, volle Leistung, LR-Modus. Antenne am U.FL nötig!
// Telegramme (Text): „D <name> <nr> <vor_s> <temp10>“ (Daten), „A <name> <nr>“ (Bestätigung).
// Keine Uhr an Bord: jeder Wert trägt „vor wie vielen Sekunden gemessen“; die Uhrzeit rechnet das Zenbook zurück
// (Befehl ZEIT nennt die Laufzeit der Basis). USB-Befehle an der Basis: DUMP (alles ausgeben), ZEIT, LEER,
// UHR <unixzeit> (stellt die Uhr; danach steht zu jedem Wert die echte Zeit im Flash – übersteht Neustarts).
// Messwert vorerst die Chiptemperatur (läuft im Wachbetrieb ~+20–30 °C über Umgebung) – echter Fühler folgt.
// Bauen: arduino-cli compile -b esp32:esp32:XIAO_ESP32C3 fuehler/c3_nachliefern  (CDCOnBoot=default = USB an)
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <LittleFS.h>

static const uint8_t ALLE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#if __has_include("geraete.h")
#include "geraete.h"                                  // eigene MAC der Basis (per .gitignore draußen)
#else
#define BASIS_MAC_WERT {0, 0, 0, 0, 0, 0}             // ohne geraete.h ist jeder C3 ein Fühler
#endif
static const uint8_t BASIS_MAC[6] = BASIS_MAC_WERT;
static const uint8_t KANAL = 1;
static const uint32_t MESSEN_MS = 60000, SENDEN_MS = 10000;
static char ich[12];
static bool basis = false;

struct Satz { uint32_t nr, t, frei; int16_t temp10, rest; };   // 16 Byte je Messwert
static uint32_t letzteNr = 0, bestaetigt = 0, gesichertBis = 0;
static volatile uint32_t ackNeu = 0;                            // aus dem Empfang: höchste fortlaufend bestätigte Nr

// ---- Basis: Eingang merken (Empfang läuft im WLAN-Task, Schreiben in loop) ----
struct Eingang { char text[120]; int rssi; };
static Eingang fifo[32];
static volatile uint8_t fKopf = 0, fFuss = 0;
static uint32_t zuletztGesehen[8];
static uint32_t uhrBasis = 0, uhrMillis = 0;      // per UHR gestellt; 0 = unbekannt
static uint32_t uhrJetzt() { return uhrBasis ? uhrBasis + (millis() - uhrMillis) / 1000 : 0; }

static void empfang(const esp_now_recv_info_t* info, const uint8_t* d, int n) {
    char z[121];
    n = n > 120 ? 120 : n;
    memcpy(z, d, n);
    z[n] = 0;
    int rssi = info && info->rx_ctrl ? info->rx_ctrl->rssi : 0;
    if (basis && z[0] == 'D') {
        uint8_t k = (fKopf + 1) % 32;
        if (k != fFuss) { strlcpy(fifo[fKopf].text, z, sizeof fifo[0].text); fifo[fKopf].rssi = rssi; fKopf = k; }
    } else if (!basis && z[0] == 'A') {
        char wer[12]; unsigned long nr;
        if (sscanf(z, "A %11s %lu", wer, &nr) == 2 && !strcmp(wer, ich) && nr == ackNeu + 1) ackNeu = nr;
    }
}

static void senden(const char* z) { esp_now_send(ALLE, (const uint8_t*)z, strlen(z)); }

// ---- Fühler ----
static void fuehlerLaden() {
    File f = LittleFS.open("/log.bin", "r");
    letzteNr = f ? f.size() / sizeof(Satz) : 0;
    if (f) f.close();
    File a = LittleFS.open("/ack.txt", "r");
    bestaetigt = a ? a.parseInt() : 0;
    if (a) a.close();
    if (bestaetigt > letzteNr) bestaetigt = letzteNr;
    ackNeu = gesichertBis = bestaetigt;
}
static void messen() {
    Satz s = {++letzteNr, (uint32_t)(millis() / 1000), 0, (int16_t)lroundf(temperatureRead() * 10), 0};
    File f = LittleFS.open("/log.bin", "a");
    if (f) { f.write((const uint8_t*)&s, sizeof s); f.close(); }
    Serial.printf("MESSE  nr %lu  temp %.1f C  · unbestaetigt %lu\n", s.nr, s.temp10 / 10.0, letzteNr - bestaetigt);
}
static void nachliefern() {
    bestaetigt = ackNeu;
    if (bestaetigt != gesichertBis) {                              // Bestätigtes merken (übersteht Stromausfall)
        File a = LittleFS.open("/ack.txt", "w");
        if (a) { a.print(bestaetigt); a.close(); }
        gesichertBis = bestaetigt;
    }
    if (bestaetigt >= letzteNr) return;
    File f = LittleFS.open("/log.bin", "r");
    if (!f) return;
    uint32_t jetzt = millis() / 1000;
    int n = 0;
    for (uint32_t nr = bestaetigt + 1; nr <= letzteNr && n < 20; nr++, n++) {
        Satz s;
        f.seek((nr - 1) * sizeof(Satz));
        if (f.read((uint8_t*)&s, sizeof s) != sizeof s) break;
        // „vor“: über einen Neustart hinweg unbekannt (Laufzeit beginnt bei 0) → dann 0 senden, nr bleibt eindeutig
        uint32_t vor = s.t <= jetzt ? jetzt - s.t : 0;
        char z[64];
        snprintf(z, sizeof z, "D %s %lu %lu %d", ich, s.nr, vor, s.temp10);
        senden(z);
        delay(15);
    }
    f.close();
    Serial.printf("SENDE  %d Werte ab nr %lu\n", n, bestaetigt + 1);
}

// ---- Basis ----
static void basisLoop() {
    while (fFuss != fKopf) {
        Eingang e = fifo[fFuss];
        fFuss = (fFuss + 1) % 32;
        char wer[12]; unsigned long nr, vor; int temp10;
        if (sscanf(e.text, "D %11s %lu %lu %d", wer, &nr, &vor, &temp10) != 4) continue;
        uint32_t h = 0;
        for (const char* p = wer; *p; p++) h = h * 31 + *p;
        uint32_t& gesehen = zuletztGesehen[h % 8];
        char gpfad[24];
        snprintf(gpfad, sizeof gpfad, "/g-%s.txt", wer);            // zuletzt gespeicherte Nr, übersteht Neustarts
        if (!gesehen) { File g = LittleFS.open(gpfad, "r"); if (g) { gesehen = g.parseInt(); g.close(); } }
        // Nur lückenlos annehmen: eine Nr nach einer Lücke weder speichern noch bestätigen – der Fühler schickt ab
        // der ersten unbestätigten Nr erneut. (Vorher: 106 vor 105 gespeichert, 105 dann als „schon gesehen“
        // bestätigt, aber nie gespeichert → Wert verloren.) Ohne Vorwissen (gesehen 0) gilt die erste Nr als Anfang.
        bool neu = !gesehen || nr == gesehen + 1;
        if (!neu && nr > gesehen) continue;                         // Lücke: abwarten
        char a[40];
        snprintf(a, sizeof a, "A %s %lu", wer, nr);
        senden(a);                                                  // Neues und Doppeltes bestätigen
        if (!neu) continue;                                         // schon gespeichert
        gesehen = nr;
        File g = LittleFS.open(gpfad, "w");
        if (g) { g.print(nr); g.close(); }
        File f = LittleFS.open("/empfang.csv", "a");
        uint32_t u = uhrJetzt();
        uint32_t gemessen = u ? u - vor : 0;                        // echte Messzeit, wenn die Uhr gestellt ist
        if (f) { f.printf("%s,%lu,%lu,%d,%d,%lu,%lu\n", wer, nr, vor, temp10, e.rssi, (unsigned long)(millis() / 1000), (unsigned long)gemessen); f.close(); }
        Serial.printf("EMPFANG  %s nr %lu  vor %lu s  temp %.1f C  rssi %d\n", wer, nr, vor, temp10 / 10.0, e.rssi);
    }
    static String befehl;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c != '\n') { befehl += c; continue; }
        befehl.trim();
        if (befehl == "DUMP") {
            Serial.println("DUMP-ANFANG wer,nr,vor_s,temp10,rssi,basis_lauf_s,unixzeit");
            File f = LittleFS.open("/empfang.csv", "r");
            while (f && f.available()) Serial.write(f.read());
            if (f) f.close();
            Serial.println("DUMP-ENDE");
        } else if (befehl == "ZEIT") Serial.printf("ZEIT basis_lauf_s %lu\n", (unsigned long)(millis() / 1000));
        else if (befehl.startsWith("UHR ")) { uhrBasis = befehl.substring(4).toInt(); uhrMillis = millis(); Serial.printf("UHR ok %lu\n", (unsigned long)uhrBasis); }
        else if (befehl == "LEER") { LittleFS.remove("/empfang.csv"); Serial.println("LEER ok"); }
        befehl = "";
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);                      // ohne Rechner am USB nicht warten (Powerbank)
    delay(1500);
    LittleFS.begin(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
    esp_wifi_set_max_tx_power(84);
    esp_wifi_set_channel(KANAL, WIFI_SECOND_CHAN_NONE);
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    basis = !memcmp(mac, BASIS_MAC, 6);
    snprintf(ich, sizeof ich, "c3-%02X%02X", mac[4], mac[5]);
    esp_now_init();
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, ALLE, 6);
    p.channel = KANAL;
    esp_now_add_peer(&p);
    esp_now_register_recv_cb(empfang);
    if (!basis) fuehlerLaden();
    Serial.printf("\n%s als %s bereit%s\n", ich, basis ? "BASIS" : "FUEHLER",
                  basis ? " · Befehle DUMP, ZEIT, LEER" : "");
    if (!basis) Serial.printf("  im Flash: %lu Werte, davon bestaetigt %lu\n", letzteNr, bestaetigt);
}

void loop() {
    if (basis) { basisLoop(); delay(5); return; }
    static uint32_t tMessen = 0, tSenden = 0;
    if (!tMessen || millis() - tMessen >= MESSEN_MS) { tMessen = millis(); messen(); }
    if (millis() - tSenden >= SENDEN_MS) { tSenden = millis(); nachliefern(); }
    delay(20);
}
