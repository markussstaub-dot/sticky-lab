// Station 8: Energie – was kostet was, und wie weit reicht der kleine Akku?
// Messläufe: Ruhe wach (Grundlast), Teil ×50, Voll ×10, Schlaf 5 min. Strom aus dem BQ27220, integriert über die Zeit.
// Wichtig: am USB misst der Chip das Netzteil mit. Für echte Werte Kabel ab, Ergebnisse stehen auf dem Display
// und in /lab/messungen.csv. KI bricht einen Lauf ab.
#pragma once
#include <esp_sleep.h>
#include <driver/gpio.h>

namespace energie {

static const int LX = 20, LY = 56, LW = 370, LH = 300;           // Akku jetzt
static const int PX = 20, PY = 372, PW = 200, PH = 96;           // Probefläche
static const int BX = 410, BY = 60, BW = 180, BH = 58, BGAP = 10; // Versuchs-Pillen 2 × 2
static const int EX = 400, EY = 200, EW = 400, EH = 280;          // Ergebnisse
static const int WX = 240, WY = 380, WW = 150, WH = 58;           // „Werte lesen“

enum Versuch : uint8_t { V_RUHE, V_TEIL, V_VOLL, V_SCHLAF, V_ANZAHL };
static const char* V_NAME[V_ANZAHL] = { "Ruhe 60 s", "Teil x50", "Voll x10", "Schlaf 5 min" };
static const char* V_CSV[V_ANZAHL] = { "ruhe-wach", "teil-x50", "voll-x10", "schlaf-leicht" };

struct Ergebnis { bool da; float maMittel; float sek; int n; float uahJe; int msJe; bool usb; };
static Ergebnis erg[V_ANZAHL] = {};
static bool probe = false;
static int laeuft = -1;

static void probeflaeche() {
    display.fillRect(PX, PY, PW, PH, probe ? SCHWARZ : WEISS);
    display.drawRect(PX, PY, PW, PH, SCHWARZ);
    text("Probe 200 x 96", PX + PW / 2, PY + PH / 2, &FreeSans9pt7b, MC_DATUM, probe ? WEISS : SCHWARZ);
}

static void zeile(const char* k, const char* w, int y) {
    text(k, LX, y, &FreeSans9pt7b);
    text(w, LX + 130, y, &FreeSans12pt7b);
}
static void akkuJetzt() {
    display.fillRect(LX, LY, LW, LH, WEISS);
    AkkuStand a = akkuLesen();
    char b[64];
    int y = LY + 8;
    snprintf(b, sizeof b, "%d mV", a.mv); zeile("Spannung", b, y); y += 34;
    snprintf(b, sizeof b, "%d mA  (Mittel %d)", a.ma, a.maMittel); zeile("Strom", b, y); y += 34;
    snprintf(b, sizeof b, "%d / %d mAh", a.rest, a.voll); zeile("Rest / voll", b, y); y += 34;
    snprintf(b, sizeof b, "%d mAh", a.design); zeile("Design", b, y); y += 34;
    snprintf(b, sizeof b, "%d %%  ·  SoH %d %%", a.soc, a.soh); zeile("Ladung", b, y); y += 34;
    snprintf(b, sizeof b, "%.1f °C  ·  %d Zyklen", a.grad, a.zyklen);
    zeile("Temperatur", b, y);
    y += 34;
    zeile("USB", usbDa() ? (laedt() ? "ja, lädt, misst mit!" : "ja, misst mit!") : "nein, Akku", y);
}

static void ergebnisse() {
    display.fillRect(EX, EY, EW, EH, WEISS);
    char b[96];
    int y = EY + 4;
    text("Ergebnisse", EX + 10, y, &FreeSansBold12pt7b); y += 34;
    for (int i = 0; i < V_ANZAHL; i++) {
        const Ergebnis& e = erg[i];
        if (!e.da) snprintf(b, sizeof b, "%s: -", V_NAME[i]);
        else if (i == V_TEIL || i == V_VOLL)
            snprintf(b, sizeof b, "%s: %.0f uAh je Bild, %d ms%s", i == V_TEIL ? "Teil" : "Voll", e.uahJe, e.msJe, e.usb ? " (USB)" : "");
        else snprintf(b, sizeof b, "%s: %.1f mA%s", i == V_RUHE ? "Ruhe wach" : "Schlaf", e.maMittel, e.usb ? " (USB)" : "");
        text(b, EX + 10, y, &FreeSans9pt7b); y += 26;
    }
    y += 8;
    int voll = bqWert(BQ_VOLL, 0);
    if (voll <= 0) { text("Hochrechnung: Kapazität unbekannt", EX + 10, y, &FreeSans9pt7b); return; }
    snprintf(b, sizeof b, "Hochrechnung, %d mAh laut Chip:", voll);
    text(b, EX + 10, y, &FreeSansBold12pt7b); y += 32;
    if (erg[V_TEIL].da && erg[V_TEIL].uahJe > 0) {
        snprintf(b, sizeof b, "%.0f Teil-Refreshes je Ladung", voll * 1000.0f / erg[V_TEIL].uahJe);
        text(b, EX + 10, y, &FreeSans9pt7b); y += 24;
    }
    if (erg[V_VOLL].da && erg[V_VOLL].uahJe > 0) {
        snprintf(b, sizeof b, "%.0f Voll-Refreshes je Ladung", voll * 1000.0f / erg[V_VOLL].uahJe);
        text(b, EX + 10, y, &FreeSans9pt7b); y += 24;
    }
    if (erg[V_RUHE].da && erg[V_RUHE].maMittel > 0) {
        snprintf(b, sizeof b, "%.0f h wach ohne Bedienung", voll / erg[V_RUHE].maMittel);
        text(b, EX + 10, y, &FreeSans9pt7b); y += 24;
    }
    if (erg[V_SCHLAF].da && erg[V_SCHLAF].maMittel > 0) {
        snprintf(b, sizeof b, "%.1f Tage im Leichtschlaf", voll / erg[V_SCHLAF].maMittel / 24.0f);
        text(b, EX + 10, y, &FreeSans9pt7b);
    }
}

static void pillen() {
    for (int i = 0; i < V_ANZAHL; i++)
        pille(BX + (i % 2) * (BW + BGAP), BY + (i / 2) * (BH + BGAP), BW, BH, V_NAME[i], i == laeuft ? R_AUSWAHL : R_WERKZEUG);
}

static void betreten() {
    statuszeile("8 · Energie");
    akkuJetzt();
    probeflaeche();
    pille(WX, WY, WW, WH, "Werte lesen", R_NEBEN);
    pillen();
    ergebnisse();
}

static bool abbruch() { return tasteUnten(K_KI); }

static void laufAnzeigen(int v) {
    laeuft = v;
    pillen();
    display.fillRect(EX, EY, EW, EH, WEISS);
    text("Messung läuft ...", EX + 10, EY + 10, &FreeSansBold12pt7b);
    text(usbDa() ? "USB steckt: misst das Netzteil mit" : "Akkubetrieb", EX + 10, EY + 50, &FreeSans9pt7b);
    text("KI bricht ab", EX + 10, EY + 76, &FreeSans9pt7b);
    teil(BX, BY, 2 * BW + BGAP, 2 * BH + BGAP);
    teil(EX, EY, EW, EH);
}

/** Schlaf: 30 × 10 s Leichtschlaf, nach jedem Wecken den Strom lesen (der Chip mittelt über die letzte Sekunde) */
static bool schlafMessen(float& maMittel, float& sek) {
    double summe = 0; int n = 0;
    uint32_t t0 = millis();
    for (int i = 0; i < 30; i++) {
        esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
        gpio_wakeup_enable((gpio_num_t)PIN_BTN_KI, tasten[K_KI].ruhe == HIGH ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        gpio_hold_en((gpio_num_t)PIN_PWR_HOLD);
        gpio_hold_en((gpio_num_t)PIN_PWR_LOCK);
        gpio_hold_en((gpio_num_t)PIN_CHG_EN_N);
        Serial.flush();
        esp_light_sleep_start();
        gpio_hold_dis((gpio_num_t)PIN_CHG_EN_N);
        gpio_hold_dis((gpio_num_t)PIN_PWR_HOLD);
        gpio_hold_dis((gpio_num_t)PIN_PWR_LOCK);
        gpio_wakeup_disable((gpio_num_t)PIN_BTN_KI);
        bool taste = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO;
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        if (taste) break;
        int16_t ma;
        if (bqLesen(BQ_STROM, ma)) { summe += ma; n++; }
    }
    sek = (millis() - t0) / 1000.0f;
    if (!n) return false;
    maMittel = fabs(summe / n);
    return n >= 5;
}

static void starte(int v) {
    laufAnzeigen(v);
    Ergebnis e = {};
    e.usb = usbDa();
    bool ok = true;
    uint32_t msSumme = 0;
    if (v == V_SCHLAF) {
        ok = schlafMessen(e.maMittel, e.sek);
    } else {
        sammelnStart();
        uint32_t t0 = millis();
        if (v == V_RUHE) {
            while (millis() - t0 < 60000) { if (abbruch()) { ok = false; break; } delay(50); }
        } else {
            int n = v == V_TEIL ? 50 : 10;
            for (int i = 0; i < n; i++) {
                if (abbruch()) { ok = false; break; }
                probe = !probe;
                probeflaeche();
                msSumme += v == V_TEIL ? teil(PX, PY, PW, PH) : voll();
                e.n++;
            }
        }
        sammelnStopp();
        e.sek = (millis() - t0) / 1000.0f;
        e.maMittel = sammelProben ? fabs(sammelMas) / e.sek : 0;
        if (!sammelProben) ok = false;
        if (e.n) {
            e.msJe = msSumme / e.n;
            // netto je Bild: Grundlast (Ruhe wach) abziehen, wenn gemessen
            double mas = fabs(sammelMas);
            if (erg[V_RUHE].da) mas -= erg[V_RUHE].maMittel * e.sek;
            e.uahJe = max(0.0, mas / 3.6 / e.n);   // 1 µAh = 3,6 mA·s
        }
    }
    laeuft = -1;
    if (ok) {
        e.da = true;
        erg[v] = e;
        char notiz[64];
        snprintf(notiz, sizeof notiz, "%.0fs %s%s", e.sek, e.usb ? "usb" : "akku", erg[V_RUHE].da ? " netto" : " brutto");
        messung("energie", V_CSV[v], lroundf(e.maMittel * 10), "0.1mA", notiz);
        if (e.n) messung("energie", V_CSV[v], lroundf(e.uahJe), "uAh/bild", notiz);
    } else {
        messung("energie", V_CSV[v], 0, "-", "abgebrochen");
        tone(PIN_BUZZER, 600, 150);
    }
    // sauberes Bild nach dem Lauf (zählt nicht mehr mit)
    display.fillScreen(WEISS);
    betreten();
    voll();
    bedient();
}

static void taste(Knopf) {}

static void tipp(int x, int y) {
    for (int i = 0; i < V_ANZAHL; i++)
        if (drin(x, y, BX + (i % 2) * (BW + BGAP), BY + (i / 2) * (BH + BGAP), BW, BH)) { starte(i); return; }
    if (drin(x, y, WX, WY, WW, WH) || drin(x, y, LX, LY, LW, LH)) { akkuJetzt(); teil(LX, LY, LW, LH); }
}

}  // namespace energie
