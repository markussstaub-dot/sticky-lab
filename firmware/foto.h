// Bildschirmfoto als PBM (P4, 1 bpp, 1 = schwarz): auf die SD nach /lab/fotos und/oder über USB
// PBM öffnen viele Bildprogramme direkt; werkzeuge/foto2png.py macht PNG daraus.
#pragma once

static const size_t FOTO_BYTES = W / 8 * H;   // 48000
static uint8_t* fotoBuf = nullptr;

static bool fotoFassen() {
    if (!fotoBuf) fotoBuf = (uint8_t*)ps_malloc(FOTO_BYTES);
    if (!fotoBuf) return false;
    memset(fotoBuf, 0, FOTO_BYTES);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (display.readPixel(x, y) != WEISS) fotoBuf[y * (W / 8) + x / 8] |= 0x80 >> (x & 7);
    return true;
}

/** Foto auf die Karte, gibt die Nummer zurück (-1 = ging nicht) */
static int fotoAufKarte() {
    if (!fotoFassen() || !sdEin()) return -1;
    char pfad[32];
    int n = 1;
    for (; n < 1000; n++) { snprintf(pfad, sizeof pfad, "/lab/fotos/f%03d.pbm", n); if (!SD.exists(pfad)) break; }
    File f = SD.open(pfad, FILE_WRITE);
    if (!f) return -1;
    f.printf("P4\n%d %d\n", W, H);
    f.write(fotoBuf, FOTO_BYTES);
    f.close();
    Serial.printf("Foto: %s\n", pfad);
    return n;
}

/** Foto über USB: Zeile „FOTO 48000“, danach der PBM-Kopf und die Rohdaten */
static void fotoSeriell() {
    if (!fotoFassen()) { Serial.println("FOTO 0"); return; }
    Serial.printf("FOTO %u\n", (unsigned)FOTO_BYTES);
    Serial.printf("P4\n%d %d\n", W, H);
    Serial.write(fotoBuf, FOTO_BYTES);
    Serial.flush();
}
