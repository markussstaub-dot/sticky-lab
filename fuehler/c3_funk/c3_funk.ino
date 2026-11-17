// c3_funk – erster Funkversuch für Station 12 „Fremdsensoren“ (sticky-lab SPEC, Idee „Sticky + XIAO ESP32-C3“).
// Jeder XIAO ESP32-C3 schickt alle 5 s per ESP-NOW (Broadcast, Kanal 1, ohne Router) eine kurze Zeile
//   „c3-CB30 temp 31.5 C lauf 120 s nr 24“
// und zeigt am USB alles, was er von anderen hört – mit Funkstärke (RSSI). Gleiche Firmware für alle Fühler.
// Zeilenformat absichtlich so, wie der Sticky es später anzeigen soll: <name> <messgröße> <wert> <einheit> …
// Bauen: arduino-cli compile -b esp32:esp32:XIAO_ESP32C3 fuehler/c3_funk   (Achtung: beim XIAO-C3 heißt „USB-Ausgabe an“ CDCOnBoot=default, „cdc“ schaltet sie AUS)
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>

static const uint8_t ALLE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t KANAL = 1;
static char name_[12];
static uint32_t nr = 0, gehoert = 0, zuletzt = 0;

static void empfang(const esp_now_recv_info_t* info, const uint8_t* d, int n) {
    char z[251];
    n = n > 250 ? 250 : n;
    memcpy(z, d, n);
    z[n] = 0;
    gehoert++;
    Serial.printf("HOERE  %s  (rssi %d dBm)\n", z, info && info->rx_ctrl ? info->rx_ctrl->rssi : 0);
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);                      // ohne Rechner am USB nicht auf ihn warten (sonst hängt er an der Powerbank)
    delay(1500);                                   // USB-CDC kurz Zeit geben
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_ps(WIFI_PS_NONE);                 // Funk nicht schlafen lassen: sonst verpasst er die meisten Rundsendungen
    // XIAO-C3 ohne Antenne am U.FL-Stecker reicht der Funk nur Zentimeter: volle Leistung und Langstrecken-Modus (LR)
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
    esp_wifi_set_max_tx_power(84);
    esp_wifi_set_channel(KANAL, WIFI_SECOND_CHAN_NONE);
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(name_, sizeof name_, "c3-%02X%02X", mac[4], mac[5]);
    if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW: Start fehlgeschlagen"); return; }
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, ALLE, 6);
    p.channel = KANAL;
    p.encrypt = false;
    esp_now_add_peer(&p);
    esp_now_register_recv_cb(empfang);
    Serial.printf("\n%s bereit · ESP-NOW Kanal %d · sendet alle 5 s\n", name_, KANAL);
}

void loop() {
    if (millis() - zuletzt >= 5000) {
        zuletzt = millis();
        char z[96];
        snprintf(z, sizeof z, "%s temp %.1f C lauf %lu s nr %lu", name_, temperatureRead(), millis() / 1000, ++nr);
        esp_err_t e = esp_now_send(ALLE, (const uint8_t*)z, strlen(z));
        Serial.printf("SENDE  %s  %s - bisher gehoert: %lu\n", z, e == ESP_OK ? "ok" : "FEHLER", gehoert);
    }
    delay(20);
}
