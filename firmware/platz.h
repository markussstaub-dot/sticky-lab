// Zwei Firmware-Plätze (SPEC „EDC und Sticky Lab auf einem Gerät“, 15.11.2026):
// EDC fest in app0, Sticky Lab fest in app1 (partitions.csv, je 4 MB). Umschalten = Startplatz setzen + Neustart.
// Dieselbe Datei liegt in EDC (reterminal-sticky/firmware-ble/platz.h); Änderungen in beiden nachziehen.
#pragma once
#include <esp_ota_ops.h>
#include <esp_partition.h>

/** Der andere App-Platz (app0 ↔ app1) */
static const esp_partition_t* platzAnderer() {
    const esp_partition_t* lauf = esp_ota_get_running_partition();
    auto sub = (lauf && lauf->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) ? ESP_PARTITION_SUBTYPE_APP_OTA_0
                                                                            : ESP_PARTITION_SUBTYPE_APP_OTA_1;
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, sub, nullptr);
}

/** Nummer des laufenden Platzes (0 oder 1) */
static int platzNummer() {
    const esp_partition_t* lauf = esp_ota_get_running_partition();
    return (lauf && lauf->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) ? 1 : 0;
}

/** Name der Firmware im anderen Platz nach der festen Regel (app0 = „EDC“, app1 = „Sticky Lab“); leer = kein gültiges
 *  Abbild. Der Projektname im Abbild taugt nicht: Arduino trägt überall denselben ein (am Gerät gesehen 15.11.2026). */
static const char* platzAndererName() {
    static const char* name = nullptr;
    if (!name) {
        const esp_partition_t* p = platzAnderer();
        esp_app_desc_t d;
        bool gueltig = p && esp_ota_get_partition_description(p, &d) == ESP_OK;
        name = !gueltig ? "" : p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 ? "Sticky Lab" : "EDC";
    }
    return name;
}

/** Auf den anderen Platz umschalten und neu starten; false = dort liegt nichts Gültiges */
static bool platzWechseln() {
    const esp_partition_t* p = platzAnderer();
    if (!p || !platzAndererName()[0]) return false;
    if (esp_ota_set_boot_partition(p) != ESP_OK) return false;
    Serial.printf("Platz: wechsle zu app%d (%s)\n", p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 ? 1 : 0, platzAndererName());
    Serial.flush();
    delay(100);
    ESP.restart();
    return true;   // nie erreicht
}

/** Notausgang: ▲ beim Einschalten 1 s gehalten → anderer Platz. Nur nach echtem Einschalten, nicht nach Neustart */
static void platzNotausgang(int pinHoch) {
    if (esp_reset_reason() != ESP_RST_POWERON) return;
    pinMode(pinHoch, INPUT_PULLUP);
    for (int i = 0; i < 20; i++) { if (digitalRead(pinHoch) != LOW) return; delay(50); }   // 1 s durchgehend gedrückt
    Serial.println("Platz: ▲ beim Einschalten gehalten");
    platzWechseln();   // kehrt nur zurück, wenn der andere Platz leer ist
}
