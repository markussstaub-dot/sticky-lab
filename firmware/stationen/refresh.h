// Station 1: Refresh-Labor
// Refresh-Art wählen (▲/▼ oder Pille), „Los“ kippt das Testfeld und misst die Dauer. „Reste sichtbar“ hält fest,
// nach wie vielen Refreshes seit dem letzten Voll-Refresh Ghosting zu sehen ist. Alles landet in /lab/messungen.csv.
#pragma once

namespace refresh {

static const int TX = 16, TY = 60, TW = 368, TH = 300;        // Testfeld
static const int VX = 400, VY = 56, VW = 384, VH = 290;        // Messwerte
static const int PY = 392, PH = 58, PW = 116, PX0 = 16, PGAP = 8;
static const int LOS_X = 690, LOS_Y = 410, LOS_R = 60;
static const int RX = 400, RY = 290, RW = 220, RH = 52;        // „Reste sichtbar“

static Art art = ART_TEIL;
static bool phase = false;
static uint32_t probe = 0;                     // Testfeld-Refreshes seit dem letzten Voll-Refresh
static long letzte[ART_ANZAHL] = { -1, -1, -1, -1 };
static long resteAb = -1; static Art resteArt = ART_TEIL;

static void testfeld() {
    display.fillRect(TX, TY, TW, TH, WEISS);
    // drei Kacheln, die bei jedem Lauf kippen: Raster, Fläche, feines Raster
    raster(TX, TY, 120, 150, phase ? 2 : 1);
    display.fillRect(TX + 124, TY, 120, 150, phase ? WEISS : SCHWARZ);
    if (phase) display.drawRect(TX + 124, TY, 120, 150, SCHWARZ);
    raster(TX + 248, TY, 120, 150, phase ? 1 : 4);
    // Probetext springt je Lauf eine Zeile weiter: Reste der alten Zeile sind Ghosting
    static const GFXfont* F[4] = { &FreeSansBold24pt7b, &FreeSansBold12pt7b, &FreeSans12pt7b, &FreeSans9pt7b };
    static const int Y[4] = { TY + 172, TY + 222, TY + 252, TY + 280 };
    int z = probe % 4;
    text(z == 0 ? "Aa 0123" : "Ghosting-Probe 0123 Ää", TX + 8, Y[z], F[z]);
    display.drawRect(TX - 1, TY - 1, TW + 2, TH + 2, SCHWARZ);
}

static void werte() {
    display.fillRect(VX, VY, VW, VH - 56, WEISS);
    char b[96];
    text("letzter Lauf", VX, VY + 6, &FreeSans9pt7b);
    long ms = letzte[art];
    if (ms >= 0) snprintf(b, sizeof b, "%ld", ms); else snprintf(b, sizeof b, "-");
    display.setFreeFont(&FreeSansBold24pt7b);
    display.setTextSize(2);
    display.setTextColor(SCHWARZ);
    display.setTextDatum(TL_DATUM);
    display.drawString(ms >= 0 ? b : "-", VX, VY + 26);
    int bw = display.textWidth(ms >= 0 ? b : "-");
    display.setTextSize(1);
    text("ms", VX + bw + 10, VY + 92, &FreeSans12pt7b, TL_DATUM);
    snprintf(b, sizeof b, "%s  ·  seit Voll: %lu", ART_NAME[art], (unsigned long)probe);
    text(b, VX, VY + 128, &FreeSans12pt7b);
    auto zahl = [](long v, char* o) { if (v >= 0) sprintf(o, "%ld", v); else strcpy(o, "-"); };
    char a[4][12];
    for (int i = 0; i < 4; i++) zahl(letzte[i], a[i]);
    snprintf(b, sizeof b, "Teil %s  ·  wach %s", a[0], a[1]);
    text(b, VX, VY + 164, &FreeSans9pt7b);
    snprintf(b, sizeof b, "Schnell %s  ·  Voll %s", a[2], a[3]);
    text(b, VX, VY + 186, &FreeSans9pt7b);
    if (resteAb >= 0) snprintf(b, sizeof b, "Reste ab %ld (%s)", resteAb, ART_NAME[resteArt]);
    else snprintf(b, sizeof b, "Reste: noch nicht gesehen");
    text(b, VX, VY + 212, &FreeSans9pt7b);
}

static void pillen() {
    display.fillRect(0, PY - 4, LOS_X - LOS_R - 10, PH + 8, WEISS);
    for (int i = 0; i < ART_ANZAHL; i++)
        pille(PX0 + i * (PW + PGAP), PY, PW, PH, ART_NAME[i], i == art ? R_AUSWAHL : R_NEBEN);
}

static void betreten() {
    statuszeile("1 · Refresh-Labor");
    testfeld();
    werte();
    pille(RX, RY, RW, RH, "Reste sichtbar", R_WERKZEUG);
    pillen();
    doppelring(LOS_X, LOS_Y, LOS_R, "Los");
}

static void los() {
    phase = !phase;
    testfeld();
    uint32_t ms = 0;
    switch (art) {
        case ART_TEIL:      ms = teil(TX - 1, TY - 1, TW + 2, TH + 2, false); break;
        case ART_TEIL_WACH: ms = teil(TX - 1, TY - 1, TW + 2, TH + 2, true); break;
        case ART_SCHNELL:   ms = schnell(); break;
        case ART_VOLL:      ms = voll(); break;
    }
    if (art == ART_VOLL) probe = 0; else probe++;
    letzte[art] = ms;
    messung("refresh", ART_NAME[art], ms, "ms");
    werte();
    teil(VX, VY, VW, VH - 56);
}

static void waehle(int a) {
    art = (Art)((a + ART_ANZAHL) % ART_ANZAHL);
    pillen();
    werte();
    teil(0, PY - 4, LOS_X - LOS_R - 10, PH + 8);
    teil(VX, VY, VW, VH - 56);
}

static void taste(Knopf k) {
    if (k == K_UP) waehle(art - 1);
    if (k == K_DOWN) waehle(art + 1);
}

static void tipp(int x, int y) {
    int dx = x - LOS_X, dy = y - LOS_Y;
    if (dx * dx + dy * dy <= LOS_R * LOS_R) { los(); return; }
    for (int i = 0; i < ART_ANZAHL; i++)
        if (drin(x, y, PX0 + i * (PW + PGAP), PY, PW, PH)) { waehle(i); return; }
    if (drin(x, y, RX, RY, RW, RH)) {
        resteAb = probe; resteArt = art;
        messung("refresh", ART_NAME[art], probe, "refreshes", "reste sichtbar");
        werte();
        teil(VX, VY, VW, VH - 56);
    }
}

}  // namespace refresh
