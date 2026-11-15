// microSD – nur der Ordner /lab/ wird angefasst (EDC-Daten auf derselben Karte bleiben in Ruhe)
// SPI-Bus wie das Display (dieselbe SPIClass, nicht neu starten), Strom über SD_PWR_EN, Erkennung über SD_DET.
// Nach reterminal-sticky FW 0.31.8. Dateiname bewusst nicht sd.h (Kollision mit der Bibliothek).
#pragma once
#include <SD.h>
#include <SPI.h>
#include "bus/Bus_SPI.h"

static bool sdDa = false;

static bool sdEin() {
    if (sdDa) return true;
    if (digitalRead(PIN_SD_DET) != LOW) return false;
    SPIClass* spi = static_cast<Bus_SPI&>(display.panel().driver().bus()).spiInstance();
    if (!spi) return false;
    digitalWrite(PIN_SD_PWR, HIGH);
    delay(20);
    sdDa = SD.begin(PIN_SD_CS, *spi, 20000000, "/sd", 5);
    if (!sdDa) digitalWrite(PIN_SD_PWR, LOW);
    if (sdDa) { SD.mkdir("/lab"); SD.mkdir("/lab/fotos"); }
    return sdDa;
}

static void sdStart() {
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    pinMode(PIN_SD_PWR, OUTPUT);
    pinMode(PIN_SD_DET, INPUT_PULLUP);
    delay(5);
    Serial.printf("SD: %s\n", sdEin() ? "eingebunden, Ordner /lab" : "keine Karte");
}

/** Eine Zeile an /lab/messungen.csv anhängen (Kopfzeile beim ersten Mal) */
static void messung(const char* station, const char* art, long wert, const char* einheit, const char* notiz = "") {
    char z[160];
    snprintf(z, sizeof z, "%lu,%s,%s,%ld,%s,%lu,%s", millis() / 1000, station, art, wert, einheit,
             (unsigned long)teilSeitVoll, notiz);
    Serial.printf("MESSUNG %s\n", z);
    if (!sdEin()) return;
    bool neu = !SD.exists("/lab/messungen.csv");
    File f = SD.open("/lab/messungen.csv", FILE_APPEND);
    if (!f) return;
    if (neu) f.println("laufzeit_s,station,art,wert,einheit,teil_seit_voll,notiz");
    f.println(z);
    f.close();
}
