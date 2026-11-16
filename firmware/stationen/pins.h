// Station 11: Pins & Neustarts – Absturzschleifen am Gerät erkennen, ohne über USB mitzulesen (Markus, 16.11.2026)
// Oben je Taste und Sensor-Pin: Pegel live, Flanken (Interrupts) pro Sekunde und Höchstwert, Interrupt-Art und ob er
// an ist, Wecken – gelesen aus den GPIO-Registern. Unten: Neustart-Grund der letzten Starts (Zähler übersteht
// Neustarts, NVS-Namensraum „lab“), wie lange der vorige Start lief, Neustarts in Folge und der Sturm-Melder
// (welcher Pin wann > 50 Interrupts/ms). Kern (Zähler, ISR, RTC-Speicher) in diagnose.h.
// Startet das Lab dreimal in Folge nach Absturz oder Watchdog neu, öffnet es gleich diese Station.
#pragma once
#include <Preferences.h>

namespace pinstation {

static const char* NAME_DE[D_ANZAHL] = { "KI", "▲", "▼", "Bewegung", "Touch-INT", "SD-Erkennung", "Laden", "USB" };
static const char* NAME_EN[D_ANZAHL] = { "AI", "▲", "▼", "Motion", "Touch INT", "SD detect", "Charging", "USB" };
static const char* CSV[D_ANZAHL] = { "pin-ki", "pin-hoch", "pin-runter", "pin-bewegung", "pin-touch-int", "pin-sd", "pin-laden", "pin-usb" };

// ---- Neustarts (NVS) -----------------------------------------------------------------------------------------
struct NstEintrag { uint32_t start; uint32_t vorherMs; uint8_t grund; uint8_t _r[3]; };   // vorherMs ~0 = unbekannt
static const int NST_N = 8;
static NstEintrag nst[NST_N];   // [0] = dieser Start
static uint32_t startNr = 0;
static uint8_t folge = 0;       // Neustarts in Folge nach Absturz/Watchdog/Unterspannung (zurück auf 0 nach 60 s Laufzeit)
static const uint32_t STABIL_MS = 60000;
static const uint32_t UNBEKANNT = 0xFFFFFFFF;

static const char* grundLang(uint8_t g) {
    static const char* de[] = { "unbekannt", "Einschalten", "Reset-Pin", "Software", "Absturz", "Interrupt-Watchdog", "Task-Watchdog",
                                "Watchdog", "Tiefschlaf", "Unterspannung", "SDIO", "USB", "JTAG", "eFuse", "Spannungseinbruch", "CPU gesperrt" };
    static const char* en[] = { "unknown", "power on", "reset pin", "software", "crash", "interrupt watchdog", "task watchdog",
                                "watchdog", "deep sleep", "brownout", "SDIO", "USB", "JTAG", "eFuse", "power glitch", "CPU lockup" };
    return g < 16 ? (EN ? en[g] : de[g]) : "?";
}
static const char* grundKurz(uint8_t g) {   // für die CSV und die Zeile „letzte Starts“
    static const char* k[] = { "?", "ein", "pin", "sw", "absturz", "int-wdt", "task-wdt", "wdt", "tief", "unterspg", "sdio", "usb", "jtag", "efuse", "glitch", "lockup" };
    return g < 16 ? k[g] : "?";
}
static bool grundSchlimm(uint8_t g) {
    return g == ESP_RST_UNKNOWN || g == ESP_RST_PANIC || g == ESP_RST_INT_WDT || g == ESP_RST_TASK_WDT || g == ESP_RST_WDT ||
           g == ESP_RST_BROWNOUT || g == ESP_RST_PWR_GLITCH || g == ESP_RST_CPU_LOCKUP;
}

static void sturmSichern() {
    Preferences p; p.begin("lab", false); p.putBytes("sturm", diagRtc.sturm, sizeof diagRtc.sturm); p.end();
}

/** Einmal in setup() nach sdStart(): Start zählen, Grund merken, Laufzeit des vorigen Starts aus dem RTC-Speicher */
static void start() {
    uint8_t grund = (uint8_t)esp_reset_reason();
    bool rtc = diagRtcGueltig();
    uint32_t vorher = rtc ? diagRtc.laufMs : UNBEKANNT;
    Preferences p;
    p.begin("lab", false);
    startNr = p.getUInt("start_n", 0) + 1;
    p.putUInt("start_n", startNr);
    folge = grundSchlimm(grund) ? min(255, p.getUChar("folge", 0) + 1) : 0;
    p.putUChar("folge", folge);
    memset(nst, 0, sizeof nst);
    p.getBytes("nst", nst, sizeof nst);
    memmove(&nst[1], &nst[0], sizeof(NstEintrag) * (NST_N - 1));
    nst[0] = { startNr, vorher, grund, {0, 0, 0} };
    p.putBytes("nst", nst, sizeof nst);
    if (!rtc) {   // nach dem Einschalten (oder Fremdinhalt von EDC): Sturm-Liste aus dem NVS
        memset(&diagRtc, 0, sizeof diagRtc);
        diagRtc.magie = DIAG_MAGIE;
        if (p.getBytes("sturm", diagRtc.sturm, sizeof diagRtc.sturm) != sizeof diagRtc.sturm) memset(diagRtc.sturm, 0, sizeof diagRtc.sturm);
        for (auto& e : diagRtc.sturm) e.gemeldet = 1;
    }
    p.end();
    diagRtc.start = startNr;
    diagRtc.laufMs = 0;
    diagRtcSiegeln();
    char notiz[64];
    if (vorher == UNBEKANNT) snprintf(notiz, sizeof notiz, "%s start %lu", grundKurz(grund), (unsigned long)startNr);
    else snprintf(notiz, sizeof notiz, "%s start %lu vorher %lu ms", grundKurz(grund), (unsigned long)startNr, (unsigned long)vorher);
    messung("pins", "neustart", folge, "in-folge", notiz);
}

// ---- Anzeige -------------------------------------------------------------------------------------------------
static const int TAB_Y = 56, ZEILE_H = 30, ZEILEN_Y = TAB_Y + 26;
static const int TAB_UNTEN = ZEILEN_Y + D_ANZAHL * ZEILE_H;   // 346
static const int X_NAME = 20, X_GPIO = 200, X_PEGEL = 290, X_RATE = 440, X_MAX = 530, X_INT = 560, X_WECK = 720;
static const int PX = 610, PW = 170, PH = 52, PY_CSV = 360, PY_LOESCHEN = 420;
static bool aktiv = false;
static uint32_t rate[D_ANZAHL], spitze[D_ANZAHL], vorher[D_ANZAHL];
static uint32_t messT = 0;
static char zuletzt[400] = "";
static bool untenNeu = false;   // Sturm oder Löschen: unteren Teil neu zeichnen

static int pegel(int pin) {
    return pin < 32 ? (REG_READ(GPIO_IN_REG) >> pin) & 1 : (REG_READ(GPIO_IN1_REG) >> (pin - 32)) & 1;
}
static void intText(int pin, char* s, size_t n) {
    static const char* de[] = { "-", "steigt", "fällt", "beide", "low", "high", "?", "?" };
    static const char* en[] = { "-", "rise", "fall", "both", "low", "high", "?", "?" };
    uint32_t r = REG_READ(GPIO_PIN0_REG + 4 * pin);
    uint32_t typ = (r >> GPIO_PIN0_INT_TYPE_S) & GPIO_PIN0_INT_TYPE_V;
    uint32_t ena = (r >> GPIO_PIN0_INT_ENA_S) & GPIO_PIN0_INT_ENA_V;
    snprintf(s, n, "%s%s", EN ? en[typ] : de[typ], typ && !ena ? T(" aus", " off") : "");
}
static bool weckt(int pin) { return (REG_READ(GPIO_PIN0_REG + 4 * pin) >> GPIO_PIN0_WAKEUP_ENABLE_S) & 1; }

static void pegelText(int i, char* s, size_t n) {
    if (i == D_USB) { snprintf(s, n, "%.1f V", analogReadMilliVolts(PIN_USB) / 1000.0f); return; }   // ADC-Pin: Spannung am Teiler
    snprintf(s, n, "%d", pegel(DIAG_GPIO[i]));
}

/** Zeichnet die Pin-Tabelle in den Puffer; gibt eine Kennung des Inhalts zurück, um unnötige Refreshs zu sparen */
static void tabelle(char* kennung, size_t n) {
    display.fillRect(0, ZEILEN_Y - 4, W, TAB_UNTEN - ZEILEN_Y + 4, WEISS);
    size_t k = 0;
    kennung[0] = 0;
    for (int i = 0; i < D_ANZAHL; i++) {
        int y = ZEILEN_Y + i * ZEILE_H + ZEILE_H / 2;
        char g[8], pg[16], r[16], m[16], it[24];
        snprintf(g, sizeof g, "%d", DIAG_GPIO[i]);
        pegelText(i, pg, sizeof pg);
        snprintf(r, sizeof r, "%lu", (unsigned long)rate[i]);
        snprintf(m, sizeof m, "%lu", (unsigned long)spitze[i]);
        intText(DIAG_GPIO[i], it, sizeof it);
        bool w = weckt(DIAG_GPIO[i]);
        const GFXfont* f = rate[i] > 0 ? &FreeSansBold9pt7b : &FreeSans9pt7b;
        text(EN ? NAME_EN[i] : NAME_DE[i], X_NAME, y, &FreeSans9pt7b, ML_DATUM);
        text(g, X_GPIO, y, &FreeSans9pt7b, ML_DATUM);
        text(pg, X_PEGEL, y, &FreeSans9pt7b, ML_DATUM);
        text(r, X_RATE, y, f, MR_DATUM);
        text(m, X_MAX, y, &FreeSans9pt7b, MR_DATUM);
        text(i == D_USB ? T("(Abfrage)", "(polled)") : it, X_INT, y, &FreeSans9pt7b, ML_DATUM);
        text(i == D_USB ? "" : w ? T("ja", "yes") : "-", X_WECK, y, &FreeSans9pt7b, ML_DATUM);
        if (i < D_ANZAHL - 1) display.drawFastHLine(X_NAME, ZEILEN_Y + (i + 1) * ZEILE_H, W - 2 * X_NAME, SCHWARZ);
        if (k < n) k += snprintf(kennung + k, n - k, "%s|%s|%s|%s|%d;", pg, r, m, it, w);
    }
}

static void unten() {
    display.fillRect(0, TAB_UNTEN + 2, W, H - TAB_UNTEN - 2, WEISS);
    display.fillRect(X_NAME, TAB_UNTEN + 4, W - 2 * X_NAME, 2, SCHWARZ);
    char z[96];
    // Zeile 1: dieser Start
    if (nst[0].vorherMs == UNBEKANNT)
        snprintf(z, sizeof z, T("Start %lu: %s", "Start %lu: %s"), (unsigned long)startNr, grundLang(nst[0].grund));
    else
        snprintf(z, sizeof z, T("Start %lu: %s, davor %.1f s gelaufen", "Start %lu: %s, ran %.1f s before"), (unsigned long)startNr,
                 grundLang(nst[0].grund), nst[0].vorherMs / 1000.0f);
    text(z, X_NAME, TAB_UNTEN + 26, &FreeSansBold9pt7b, ML_DATUM);
    // Zeile 2: in Folge
    if (folge >= 2) snprintf(z, sizeof z, T("NEUSTARTSCHLEIFE: %d Neustarts in Folge (< 60 s)", "RESTART LOOP: %d restarts in a row (< 60 s)"), folge);
    else snprintf(z, sizeof z, T("Neustarts in Folge: %d", "Restarts in a row: %d"), folge);
    text(z, X_NAME, TAB_UNTEN + 52, folge >= 2 ? &FreeSansBold9pt7b : &FreeSans9pt7b, ML_DATUM);
    // Zeile 3: die Starts davor (neu → alt)
    int k = snprintf(z, sizeof z, T("davor: ", "before: "));
    for (int i = 1; i < 6 && nst[i].start && k < (int)sizeof z; i++) k += snprintf(z + k, sizeof z - k, "%s%s", i > 1 ? " · " : "", grundKurz(nst[i].grund));
    if (!nst[1].start) snprintf(z + k, sizeof z - k, "-");
    text(z, X_NAME, TAB_UNTEN + 78, &FreeSans9pt7b, ML_DATUM);
    // Zeile 4: Sturm-Melder, jüngster Eintrag
    const SturmEintrag* s = nullptr;
    for (int i = 1; i <= STURM_N; i++) {
        const SturmEintrag& e = diagRtc.sturm[(diagRtc.sturmNeu + STURM_N - i) % STURM_N];
        if (e.start) { s = &e; break; }
    }
    if (s) snprintf(z, sizeof z, T("Sturm: GPIO%d, Start %lu nach %.1f s", "Storm: GPIO%d, start %lu after %.1f s"), s->pin, (unsigned long)s->start, s->ms / 1000.0f);
    else snprintf(z, sizeof z, T("Sturm-Melder: noch keiner (> 50 Interrupts/ms)", "Storm detector: none yet (> 50 interrupts/ms)"));
    text(z, X_NAME, TAB_UNTEN + 104, s ? &FreeSansBold9pt7b : &FreeSans9pt7b, ML_DATUM);
    pille(PX, PY_CSV, PW, PH, T("In CSV", "To CSV"), R_WERKZEUG);
    pille(PX, PY_LOESCHEN, PW, PH, T("Löschen", "Clear"), R_NEBEN);
}

static void kopf() {
    text("Pin", X_NAME, TAB_Y + 10, &FreeSansBold9pt7b, ML_DATUM);
    text("GPIO", X_GPIO, TAB_Y + 10, &FreeSansBold9pt7b, ML_DATUM);
    text(T("Pegel", "Level"), X_PEGEL, TAB_Y + 10, &FreeSansBold9pt7b, ML_DATUM);
    text(T("Fl./s", "Edg/s"), X_RATE, TAB_Y + 10, &FreeSansBold9pt7b, MR_DATUM);
    text("max", X_MAX, TAB_Y + 10, &FreeSansBold9pt7b, MR_DATUM);
    text("Interrupt", X_INT, TAB_Y + 10, &FreeSansBold9pt7b, ML_DATUM);
    text(T("Weckt", "Wakes"), X_WECK, TAB_Y + 10, &FreeSansBold9pt7b, ML_DATUM);
}

// Die Pins ohne eigene ISR zählen nur, solange die Station offen ist (jede Flanke, Sturmbremse inklusive)
static void scharf() {
    if (aktiv) return;
    aktiv = true;
    pinMode(PIN_6D_INT, INPUT_PULLUP);   // offener Drain; das Lab richtet den Pin sonst nicht ein
    for (int i = D_BEWEGUNG; i <= D_LADEN; i++) attachInterruptArg(digitalPinToInterrupt(DIAG_GPIO[i]), diagIsr, (void*)(intptr_t)i, CHANGE);
}
static void verlassen() {
    if (!aktiv) return;
    aktiv = false;
    for (int i = D_BEWEGUNG; i <= D_LADEN; i++) detachInterrupt(digitalPinToInterrupt(DIAG_GPIO[i]));
}

static void betreten() {
    statuszeile(T("11 · Pins & Neustarts", "11 · Pins & restarts"));
    scharf();
    for (int i = 0; i < D_ANZAHL; i++) { vorher[i] = diagFlanken[i]; rate[i] = 0; spitze[i] = 0; }
    messT = millis();
    kopf();
    tabelle(zuletzt, sizeof zuletzt);
    unten();
}

static void tick() {
    if (untenNeu) { untenNeu = false; unten(); teil(0, TAB_UNTEN + 2, W, H - TAB_UNTEN - 2); }
    if (millis() - messT < 1000) return;
    float s = (millis() - messT) / 1000.0f;
    messT = millis();
    for (int i = 0; i < D_ANZAHL; i++) {
        uint32_t f = diagFlanken[i];
        rate[i] = (uint32_t)((f - vorher[i]) / s + 0.5f);
        vorher[i] = f;
        if (rate[i] > spitze[i]) spitze[i] = rate[i];
    }
    char neu[sizeof zuletzt];
    tabelle(neu, sizeof neu);
    if (strcmp(neu, zuletzt) == 0) return;   // Puffer ist gleich wie auf dem Panel
    strcpy(zuletzt, neu);
    teil(0, ZEILEN_Y - 4, W, TAB_UNTEN - ZEILEN_Y + 4);
}

static void taste(Knopf) {}   // ▲▼ tun nichts – drücken und in der Tabelle zusehen

static void tipp(int x, int y) {
    if (drin(x, y, PX, PY_CSV, PW, PH)) {
        pille(PX, PY_CSV, PW, PH, T("In CSV", "To CSV"), R_WERKZEUG, true); teil(PX, PY_CSV, PW, PH);
        for (int i = 0; i < D_ANZAHL; i++) {
            char notiz[48], it[24];
            intText(DIAG_GPIO[i], it, sizeof it);
            snprintf(notiz, sizeof notiz, "gpio %d pegel %d max %lu int %s%s", DIAG_GPIO[i], pegel(DIAG_GPIO[i]), (unsigned long)spitze[i],
                     i == D_USB ? "abfrage" : it, weckt(DIAG_GPIO[i]) ? " weckt" : "");
            messung("pins", CSV[i], rate[i], "flanken/s", notiz);
        }
        tone(PIN_BUZZER, 2200, 40);
        pille(PX, PY_CSV, PW, PH, T("In CSV", "To CSV"), R_WERKZEUG); teil(PX, PY_CSV, PW, PH);
    } else if (drin(x, y, PX, PY_LOESCHEN, PW, PH)) {
        // Verlauf, Zähler „in Folge“ und Sturm-Liste leeren; die Start-Nummer läuft weiter
        folge = 0;
        memset(&nst[1], 0, sizeof(NstEintrag) * (NST_N - 1));
        memset(diagRtc.sturm, 0, sizeof diagRtc.sturm);
        diagRtc.sturmNeu = 0;
        diagRtcSiegeln();
        Preferences p; p.begin("lab", false);
        p.putUChar("folge", 0); p.putBytes("nst", nst, sizeof nst); p.putBytes("sturm", diagRtc.sturm, sizeof diagRtc.sturm);
        p.end();
        for (int i = 0; i < D_ANZAHL; i++) spitze[i] = 0;
        messung("pins", "geloescht", startNr, "start");
        unten(); teil(0, TAB_UNTEN + 2, W, H - TAB_UNTEN - 2);
    }
}

}  // namespace pinstation

/** In jeder Schleife (auch außerhalb der Station): Laufzeit in den RTC-Speicher, USB-Wechsel zählen, Stürme melden */
static void diagPoll() {
    portENTER_CRITICAL(&diagMux);   // die ISR siegelt den RTC-Speicher auch
    diagRtc.laufMs = millis();
    diagRtcSiegeln();
    portEXIT_CRITICAL(&diagMux);
    static bool usb = usbDa();
    bool u = usbDa();
    if (u != usb) { usb = u; diagFlanken[D_USB]++; }
    if (pinstation::folge && millis() > pinstation::STABIL_MS) {   // lief eine Minute: die Schleife ist vorbei
        pinstation::folge = 0;
        Preferences p; p.begin("lab", false); p.putUChar("folge", 0); p.end();
        messung("pins", "stabil", pinstation::startNr, "start");
        pinstation::untenNeu = true;
    }
    bool neu = false;
    for (auto& e : diagRtc.sturm) {
        if (!e.start || e.gemeldet) continue;
        e.gemeldet = 1; neu = true;
        char notiz[48];
        snprintf(notiz, sizeof notiz, "start %lu nach %lu ms, interrupt aus", (unsigned long)e.start, (unsigned long)e.ms);
        messung("pins", "sturm", e.pin, "gpio", notiz);
        tone(PIN_BUZZER, 600, 120);
    }
    if (neu) { portENTER_CRITICAL(&diagMux); diagRtcSiegeln(); portEXIT_CRITICAL(&diagMux); }
    if (neu) { pinstation::sturmSichern(); pinstation::untenNeu = true; }
}
