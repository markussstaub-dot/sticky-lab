// Diagnose-Kern für Station 11 „Pins & Neustarts“: Flanken-Zähler je Pin, Sturm-Melder und Laufzeit, die einen
// Neustart übersteht. Liegt vor eingabe.h, weil die Tasten-ISR hier mitzählt. Anzeige, NVS und CSV: stationen/pins.h
// Anlass: EDC 0.39.1 startete nach dem Wechsel aus dem Lab im Sekundentakt neu (Tasten-Interrupt-Sturm → Interrupt-
// Watchdog). Ziel: so eine Schleife am Gerät erkennen, ohne über USB mitzulesen.
#pragma once
#include <esp_attr.h>
#include <esp_timer.h>
#include <soc/gpio_reg.h>

// Index = Zeile der Station; 0..2 = Knopf-Nummer (K_KI, K_UP, K_DOWN), damit die Tasten-ISR direkt zählen kann
enum DiagPin : uint8_t { D_KI, D_UP, D_DOWN, D_BEWEGUNG, D_TOUCH, D_SD, D_LADEN, D_USB, D_ANZAHL };
static const int DIAG_GPIO[D_ANZAHL] = { PIN_BTN_KI, PIN_BTN_UP, PIN_BTN_DOWN, 7, PIN_TP_INT, PIN_SD_DET, PIN_CHG_STATE, PIN_USB };
static const int PIN_6D_INT = 7;   // LSM6DS3 INT1 (6D_INTn), offener Drain, aktiv low – wie EDC sensoren.h
static const uint32_t STURM_PRO_MS = 50;   // mehr Interrupts eines Pins in derselben Millisekunde = Sturm

static volatile uint32_t diagFlanken[D_ANZAHL];   // gezählte Interrupts (USB: Pegelwechsel aus der Abfrage)
static volatile uint32_t diagSturmT[D_ANZAHL];
static volatile uint16_t diagSturmN[D_ANZAHL];

// RTC-Speicher ohne Init: übersteht Software-, Absturz- und Watchdog-Neustarts, nicht das Ausschalten.
// EDC im anderen Platz kann denselben Bereich belegen – darum Magie und Prüfsumme.
struct SturmEintrag { uint32_t start; uint32_t ms; uint8_t pin; uint8_t gemeldet; uint16_t _r; };
static const int STURM_N = 4;
struct DiagRtc {
    uint32_t magie;
    uint32_t laufMs;      // Laufzeit des aktuellen Starts, laufend nachgeführt
    uint32_t start;       // Start-Nummer (aus NVS)
    SturmEintrag sturm[STURM_N];
    uint8_t sturmNeu;     // Ring-Schreibstelle
    uint8_t _r[3];
    uint32_t pruef;
};
static RTC_NOINIT_ATTR DiagRtc diagRtc;
static portMUX_TYPE diagMux = portMUX_INITIALIZER_UNLOCKED;
static const uint32_t DIAG_MAGIE = 0x4C414231;   // „LAB1“

static uint32_t IRAM_ATTR diagPruefsumme() {
    const uint32_t* p = (const uint32_t*)&diagRtc;
    uint32_t s = 0x5A5A5A5A;
    for (size_t i = 0; i < offsetof(DiagRtc, pruef) / 4; i++) s = (s << 5 | s >> 27) ^ p[i];
    return s;
}
static bool diagRtcGueltig() { return diagRtc.magie == DIAG_MAGIE && diagRtc.pruef == diagPruefsumme(); }
static void diagRtcSiegeln() { diagRtc.pruef = diagPruefsumme(); }

/** Aus jeder Pin-ISR: zählen und bei Sturm den Interrupt dieses Pins abschalten (Abfrage läuft weiter) */
static void IRAM_ATTR diagFlanke(int i) {
    diagFlanken[i]++;
    uint32_t jetzt = (uint32_t)(esp_timer_get_time() / 1000);
    if (jetzt != diagSturmT[i]) { diagSturmT[i] = jetzt; diagSturmN[i] = 0; }
    if (++diagSturmN[i] > STURM_PRO_MS) {
        int pin = DIAG_GPIO[i];
        REG_CLR_BIT(GPIO_PIN0_REG + 4 * pin, GPIO_PIN0_INT_ENA_M);
        diagSturmN[i] = 0;
        portENTER_CRITICAL_ISR(&diagMux);
        SturmEintrag& e = diagRtc.sturm[diagRtc.sturmNeu % STURM_N];
        e.start = diagRtc.start; e.ms = jetzt; e.pin = (uint8_t)pin; e.gemeldet = 0;
        diagRtc.sturmNeu = (diagRtc.sturmNeu + 1) % STURM_N;
        diagRtc.pruef = diagPruefsumme();
        portEXIT_CRITICAL_ISR(&diagMux);
    }
}
static void IRAM_ATTR diagIsr(void* arg) { diagFlanke((int)(intptr_t)arg); }
