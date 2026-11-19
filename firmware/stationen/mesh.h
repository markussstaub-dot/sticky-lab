// Station 13: Mesh – der Sticky spricht direkt per BLE mit einem Meshtastic-Knoten (T1000-E), ohne Handy.
// Durchstich (19.11.2026): verbinden (PIN 123456), Knoten und Kanäle lesen, Textnachrichten empfangen und
// Schnellantworten senden. Plan: ../SPEC.md Station 13, Ideen: t1000e-meshtastic/IDEEN.md.
//
// Meshtastic-BLE: Dienst 6ba1b218…, ToRadio (schreiben), FromRadio (lesen bis leer), FromNum (Meldung „es liegt was an“).
// Nachrichten sind Protobuf (meshtastic/protobufs, mesh.proto); hier ein kleiner Leser nur für die nötigen Felder.
// Meshtastic erlaubt eine BLE-Verbindung: solange die Station verbunden ist, kommt die App am Handy nicht dran.
// Station verlassen trennt.
#pragma once
#include <NimBLEDevice.h>

namespace mesh {

static const char* SVC = "6ba1b218-15a8-461f-9fa8-5dcae273eafd";
static const char* TORADIO = "f75c76d2-129e-4dad-a1dd-7866124401e7";
static const char* FROMRADIO = "2c55e69e-4993-11ed-b878-0242ac120002";
static const char* FROMNUM = "ed9da18c-a800-4f66-a670-aa7547e34453";
static const uint32_t PIN_CODE = 123456;   // Werks-PIN des T1000-E (bluetooth.fixedPin)

// ---- Protobuf lesen (nur Varint, fixed32 und Längenfelder) -------------------------------------------------
struct Pb {
    const uint8_t* p; const uint8_t* e;
    bool ende() const { return p >= e; }
    uint64_t varint() { uint64_t v = 0; int s = 0; while (p < e) { uint8_t b = *p++; v |= (uint64_t)(b & 0x7F) << s; if (!(b & 0x80)) break; s += 7; } return v; }
    uint32_t fix32() { uint32_t v = 0; if (e - p >= 4) { memcpy(&v, p, 4); p += 4; } else p = e; return v; }
    Pb teil(size_t n) { Pb t{ p, p + min((size_t)(e - p), n) }; p = t.e; return t; }
    // nächstes Feld: Nummer und Typ; Werte holt der Aufrufer oder überspringt sie
    bool feld(uint32_t& nr, uint8_t& typ) { if (ende()) return false; uint64_t k = varint(); nr = k >> 3; typ = k & 7; return true; }
    void ueber(uint8_t typ) {
        if (typ == 0) varint(); else if (typ == 5) fix32(); else if (typ == 1) p = min(e, p + 8);
        else if (typ == 2) { size_t n = varint(); p = min(e, p + n); } else p = e;
    }
    Pb laenge() { size_t n = varint(); return teil(n); }
    void str(char* z, size_t n) { Pb t = laenge(); size_t k = min(n - 1, (size_t)(t.e - t.p)); memcpy(z, t.p, k); z[k] = 0; }
};

// ---- Zustand ------------------------------------------------------------------------------------------------
enum Lage : uint8_t { L_AUS, L_SUCHE, L_VERBINDE, L_KOPPLE, L_LESE, L_BEREIT, L_FEHLER };
static Lage lage = L_AUS;
static char fehler[48] = "";
static uint32_t ichNr = 0;
static bool bereitGemeldet = false;

struct Knoten { uint32_t nr; char kurz[8]; char lang[40]; int32_t lat, lon; int akku; float snr; uint32_t gehoert; };   // gehoert = millis()
static Knoten knoten[12];
static int knotenN = 0;
static Knoten* knotenFuer(uint32_t nr, bool neu = true) {
    for (int i = 0; i < knotenN; i++) if (knoten[i].nr == nr) return &knoten[i];
    if (!neu) return nullptr;
    int i = knotenN < 12 ? knotenN++ : 11;
    knoten[i] = Knoten{ nr, "", "", 0, 0, -1, 0, 0 };
    snprintf(knoten[i].kurz, sizeof knoten[i].kurz, "%04x", (unsigned)(nr & 0xFFFF));
    return &knoten[i];
}

static char kanalName[8][16];
static int kanalN = 0;
static int sendeKanal = 0;   // Index am Knoten; nach dem Lesen „Familie“, falls vorhanden

struct Nachricht { uint32_t von; uint8_t kanal; char text[96]; int rssi; float snr; uint32_t um; bool eigen; };
static Nachricht nachr[5];   // neueste vorne
static int nachrN = 0;
static void merken(const Nachricht& n) {
    for (int i = min(nachrN, 4); i > 0; i--) nachr[i] = nachr[i - 1];
    nachr[0] = n;
    if (nachrN < 5) nachrN++;
}

static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic *chTo = nullptr, *chFrom = nullptr, *chNum = nullptr;
static volatile bool anliegend = false;    // FromNum hat gemeldet
static volatile bool getrennt = false;
static bool neuZeichnen = false;
static uint32_t verbundenSeit = 0;

class Rueckrufe : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient*, int grund) override { getrennt = true; Serial.printf("MESH getrennt %d\n", grund); }
    void onPassKeyEntry(NimBLEConnInfo& info) override { Serial.println("MESH PIN"); NimBLEDevice::injectPassKey(info, PIN_CODE); }
    void onConfirmPasskey(NimBLEConnInfo& info, uint32_t) override { NimBLEDevice::injectConfirmPasskey(info, true); }
    void onAuthenticationComplete(NimBLEConnInfo& info) override { Serial.printf("MESH gekoppelt verschl %d auth %d bond %d\n", info.isEncrypted(), info.isAuthenticated(), info.isBonded()); }
};
static Rueckrufe rueckrufe;

// ---- Nachrichten auswerten ----------------------------------------------------------------------------------
static void position(Pb b, Knoten* k) {
    uint32_t nr; uint8_t t;
    while (b.feld(nr, t)) {
        if (nr == 1 && t == 5) k->lat = (int32_t)b.fix32();
        else if (nr == 2 && t == 5) k->lon = (int32_t)b.fix32();
        else b.ueber(t);
    }
}
static void nutzer(Pb b, Knoten* k) {
    uint32_t nr; uint8_t t;
    while (b.feld(nr, t)) {
        if (nr == 2 && t == 2) b.str(k->lang, sizeof k->lang);
        else if (nr == 3 && t == 2) b.str(k->kurz, sizeof k->kurz);
        else b.ueber(t);
    }
}
static void knotenInfo(Pb b) {
    uint32_t nr; uint8_t t; Knoten tmp{}; Pb u{}, pos{}, met{}; bool hatU = false, hatP = false, hatM = false;
    while (b.feld(nr, t)) {
        if (nr == 1 && t == 0) tmp.nr = b.varint();
        else if (nr == 2 && t == 2) { u = b.laenge(); hatU = true; }
        else if (nr == 3 && t == 2) { pos = b.laenge(); hatP = true; }
        else if (nr == 4 && t == 5) { uint32_t f = b.fix32(); memcpy(&tmp.snr, &f, 4); }
        else if (nr == 6 && t == 2) { met = b.laenge(); hatM = true; }
        else b.ueber(t);
    }
    if (!tmp.nr) return;
    Knoten* k = knotenFuer(tmp.nr);
    if (hatU) nutzer(u, k);
    if (hatP) position(pos, k);
    if (hatM) { uint32_t n2; uint8_t t2; while (met.feld(n2, t2)) { if (n2 == 1 && t2 == 0) k->akku = met.varint(); else met.ueber(t2); } }
}
static void kanal(Pb b) {
    uint32_t nr; uint8_t t; int idx = 0, rolle = 0; char name[16] = "";
    while (b.feld(nr, t)) {
        if (nr == 1 && t == 0) idx = b.varint();
        else if (nr == 2 && t == 2) { Pb s = b.laenge(); uint32_t n2; uint8_t t2; while (s.feld(n2, t2)) { if (n2 == 3 && t2 == 2) s.str(name, sizeof name); else s.ueber(t2); } }
        else if (nr == 3 && t == 0) rolle = b.varint();
        else b.ueber(t);
    }
    if (idx < 0 || idx >= 8 || rolle == 0) return;   // Rolle 0 = abgeschaltet
    snprintf(kanalName[idx], sizeof kanalName[idx], "%s", name[0] ? name : (idx == 0 ? "LongFast" : "?"));
    kanalN = max(kanalN, idx + 1);
    if (!strcmp(name, "Familie")) sendeKanal = idx;
}
static void paket(Pb b) {
    uint32_t nr; uint8_t t, kan = 0; uint32_t von = 0; int rssi = 0; float snr = 0; int port = 0; Pb nutz{}; bool hat = false;
    while (b.feld(nr, t)) {
        if (nr == 1 && t == 5) von = b.fix32();
        else if (nr == 3 && t == 0) kan = b.varint();
        else if (nr == 4 && t == 2) {
            Pb d = b.laenge(); uint32_t n2; uint8_t t2;
            while (d.feld(n2, t2)) { if (n2 == 1 && t2 == 0) port = d.varint(); else if (n2 == 2 && t2 == 2) { nutz = d.laenge(); hat = true; } else d.ueber(t2); }
        }
        else if (nr == 8 && t == 5) { uint32_t f = b.fix32(); memcpy(&snr, &f, 4); }
        else if (nr == 12 && t == 0) rssi = (int32_t)b.varint();
        else b.ueber(t);
    }
    Knoten* k = knotenFuer(von);
    k->gehoert = millis();
    if (snr) k->snr = snr;
    if (!hat) return;   // verschlüsselt für einen fremden Kanal
    if (port == 1) {    // TEXT_MESSAGE_APP
        Nachricht n{ von, kan, "", rssi, snr, millis(), von == ichNr };
        size_t len = min(sizeof n.text - 1, (size_t)(nutz.e - nutz.p));
        memcpy(n.text, nutz.p, len); n.text[len] = 0;
        merken(n);
        neuZeichnen = true;
        tone(PIN_BUZZER, 2400, 60); delay(90); tone(PIN_BUZZER, 3200, 80);
        Serial.printf("MESH text von %s kanal %u rssi %d snr %.1f: %s\n", k->kurz, kan, rssi, snr, n.text);
        messung("mesh", "text", rssi, "dBm", k->kurz);
    } else if (port == 3) { position(nutz, k); neuZeichnen = true; }   // POSITION_APP
    else if (port == 4) { nutzer(nutz, k); neuZeichnen = true; }        // NODEINFO_APP
}
static void fromRadio(const uint8_t* d, size_t n) {
    Pb b{ d, d + n }; uint32_t nr; uint8_t t;
    while (b.feld(nr, t)) {
        if (nr == 2 && t == 2) paket(b.laenge());
        else if (nr == 3 && t == 2) { Pb m = b.laenge(); uint32_t n2; uint8_t t2; while (m.feld(n2, t2)) { if (n2 == 1 && t2 == 0) ichNr = m.varint(); else m.ueber(t2); } }
        else if (nr == 4 && t == 2) knotenInfo(b.laenge());
        else if (nr == 10 && t == 2) kanal(b.laenge());
        else if (nr == 7 && t == 0) { b.varint(); lage = L_BEREIT; neuZeichnen = true; }   // config_complete_id
        else if (nr == 11 && t == 2) {   // queueStatus: Antwort auf unser Senden
            Pb q = b.laenge(); uint32_t n2; uint8_t t2; int res = 0;
            while (q.feld(n2, t2)) { if (n2 == 1 && t2 == 0) res = (int32_t)q.varint(); else q.ueber(t2); }
            Serial.printf("MESH warteschlange res %d\n", res);
        }
        else b.ueber(t);
    }
}
/** FromRadio lesen, bis es leer ist; gibt die Zahl der Pakete zurück */
static int abholen(int max = 64) {
    int n = 0;
    while (chFrom && n < max) {
        NimBLEAttValue v = chFrom->readValue();
        if (v.size() == 0) break;
        fromRadio(v.data(), v.size());
        n++;
    }
    return n;
}

// ---- Protobuf schreiben ----------------------------------------------------------------------------------------
static size_t vschreib(uint8_t* p, uint64_t v) { size_t i = 0; do { uint8_t b = v & 0x7F; v >>= 7; p[i++] = b | (v ? 0x80 : 0); } while (v); return i; }

static bool senden(const char* textUtf8) {
    if (lage != L_BEREIT || !chTo) return false;
    uint8_t daten[160], pkt[200], to[220];
    size_t len = min(strlen(textUtf8), (size_t)120), d = 0, k = 0, n = 0;
    daten[d++] = 0x08; daten[d++] = 1;                         // Data.portnum = TEXT_MESSAGE_APP
    daten[d++] = 0x12; d += vschreib(daten + d, len); memcpy(daten + d, textUtf8, len); d += len;   // Data.payload
    pkt[k++] = 0x15; uint32_t alle = 0xFFFFFFFF; memcpy(pkt + k, &alle, 4); k += 4;               // to = Rundruf
    pkt[k++] = 0x18; k += vschreib(pkt + k, sendeKanal);                                          // channel
    pkt[k++] = 0x22; k += vschreib(pkt + k, d); memcpy(pkt + k, daten, d); k += d;                // decoded
    pkt[k++] = 0x35; uint32_t id = esp_random(); memcpy(pkt + k, &id, 4); k += 4;                 // id
    pkt[k++] = 0x48; k += vschreib(pkt + k, 3);   // hop_limit 3 (wie die App); ohne ging es mit 0 raus = keine Weiterleitung
    to[n++] = 0x0A; n += vschreib(to + n, k); memcpy(to + n, pkt, k); n += k;                     // ToRadio.packet
    bool ok = chTo->writeValue(to, n, true);
    Serial.printf("MESH senden kanal %d ok %d: %s\n", sendeKanal, ok, textUtf8);
    if (ok) {
        Nachricht m{ ichNr, (uint8_t)sendeKanal, "", 0, 0, millis(), true };
        snprintf(m.text, sizeof m.text, "%s", textUtf8);
        merken(m);
        messung("mesh", "gesendet", (long)len, "byte", kanalName[sendeKanal]);
    }
    return ok;
}

// ---- Verbindung (blockiert jeweils einige Sekunden; die Station zeigt vorher, was gerade passiert) -------------
static void zeichneLage();
static void setzeLage(Lage l, const char* f = "") { lage = l; snprintf(fehler, sizeof fehler, "%s", f); zeichneLage(); }

static void trennen() {
    if (client && client->isConnected()) client->disconnect();
    chTo = chFrom = chNum = nullptr;
    lage = L_AUS;
}

static void verbinden() {
    static bool init = false;
    if (!init) {
        NimBLEDevice::init("Sticky-Lab");
        NimBLEDevice::setSecurityAuth(true, true, true);            // Bonding (Meshtastic trennt sonst nach dem PIN; EDC nutzt keine Bonds), MITM, SC
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY);    // wir „tippen“ den PIN ein
        NimBLEDevice::setMTU(512);
        init = true;
    }
    uint32_t t0 = millis();
    setzeLage(L_SUCHE);
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    NimBLEScanResults r = scan->getResults(5000, false);
    const NimBLEAdvertisedDevice* ziel = nullptr;
    for (int i = 0; i < (int)r.getCount(); i++) {
        const NimBLEAdvertisedDevice* d = r.getDevice(i);
        if (d->isAdvertisingService(NimBLEUUID(SVC))) { ziel = d; break; }
    }
    Serial.printf("MESH suche: %d Geräte, Meshtastic %s\n", (int)r.getCount(), ziel ? ziel->getName().c_str() : "keins");
    if (!ziel) { setzeLage(L_FEHLER, T("kein Meshtastic-Knoten in Reichweite", "no Meshtastic node in range")); return; }

    setzeLage(L_VERBINDE);
    if (!client) { client = NimBLEDevice::createClient(); client->setClientCallbacks(&rueckrufe, false); client->setConnectTimeout(8000); }
    getrennt = false;
    if (!client->connect(ziel)) { setzeLage(L_FEHLER, T("Verbinden fehlgeschlagen", "connect failed")); return; }
    setzeLage(L_KOPPLE);
    if (!client->secureConnection()) { trennen(); setzeLage(L_FEHLER, T("Koppeln fehlgeschlagen (PIN?)", "pairing failed (PIN?)")); return; }
    NimBLERemoteService* s = client->getService(SVC);
    if (s) { chTo = s->getCharacteristic(TORADIO); chFrom = s->getCharacteristic(FROMRADIO); chNum = s->getCharacteristic(FROMNUM); }
    if (!chTo || !chFrom || !chNum) { trennen(); setzeLage(L_FEHLER, T("Meshtastic-Dienst unvollständig", "Meshtastic service incomplete")); return; }
    chNum->subscribe(true, [](NimBLERemoteCharacteristic*, uint8_t*, size_t, bool) { anliegend = true; });

    setzeLage(L_LESE);
    knotenN = 0; kanalN = 0; sendeKanal = 0; ichNr = 0;
    uint8_t w[8]; size_t n = 0; w[n++] = 0x18; n += vschreib(w + n, esp_random() & 0x7FFFFFFF);   // want_config_id
    chTo->writeValue(w, n, true);
    int pakete = 0;
    for (int runde = 0; runde < 20 && lage != L_BEREIT; runde++) pakete += abholen(200);
    if (lage != L_BEREIT) { trennen(); setzeLage(L_FEHLER, T("Einstellungen kamen nicht an", "config did not arrive")); return; }
    verbundenSeit = millis();
    uint32_t ms = millis() - t0;
    Serial.printf("MESH bereit nach %lu ms, %d Pakete, %d Knoten, %d Kanäle, senden auf %d\n", (unsigned long)ms, pakete, knotenN, kanalN, sendeKanal);
    messung("mesh", "verbunden", ms, "ms", ziel->getName().c_str());
    tone(PIN_BUZZER, 2200, 40); delay(70); tone(PIN_BUZZER, 3000, 60);
}

// ---- Seite ----------------------------------------------------------------------------------------------------
static const int ZY = 54, ZH = 50;                       // Verbindungszeile
static const int NX = 20, NY = 116, NW = 500, NH = 272;  // Nachrichten
static const int KX = 540, KY = 116, KW2 = 240;          // Knoten
static const int AY = 404, AH = 58;                      // Antworten
static const char* ANTWORT_DE[4] = { "OK", "Komme", "Bin da", "Hilfe!" };
static const char* ANTWORT_EN[4] = { "OK", "On my way", "I'm here", "Help!" };
static const int AX[4] = { 20, 136, 272, 398 }, AW[4] = { 104, 124, 114, 114 };
static const int CX = 524, CW = 256;

static void alter(uint32_t um, char* z, size_t n) {
    uint32_t s = (millis() - um) / 1000;
    if (s < 60) snprintf(z, n, T("gerade", "just now"));
    else if (s < 3600) snprintf(z, n, T("vor %lu min", "%lu min ago"), (unsigned long)(s / 60));
    else snprintf(z, n, T("vor %lu h", "%lu h ago"), (unsigned long)(s / 3600));
}

static void zeichneZeile() {
    display.fillRect(0, ZY, W, ZH, WEISS);
    char z[80];
    Knoten* ich = ichNr ? knotenFuer(ichNr, false) : nullptr;
    switch (lage) {
        case L_AUS: snprintf(z, sizeof z, T("nicht verbunden  ·  ▲ = verbinden", "not connected  ·  ▲ = connect")); break;
        case L_SUCHE: snprintf(z, sizeof z, T("suche Meshtastic-Knoten …", "scanning for Meshtastic node …")); break;
        case L_VERBINDE: snprintf(z, sizeof z, T("verbinde …", "connecting …")); break;
        case L_KOPPLE: snprintf(z, sizeof z, T("kopple mit PIN …", "pairing with PIN …")); break;
        case L_LESE: snprintf(z, sizeof z, T("lese Knoten und Kanäle …", "reading nodes and channels …")); break;
        case L_FEHLER: snprintf(z, sizeof z, "%s  ·  %s", fehler, T("▲ = nochmal", "▲ = retry")); break;
        case L_BEREIT:
            if (ich && ich->akku >= 0) snprintf(z, sizeof z, "%s  ·  %s  ·  %d %%", ich->lang[0] ? ich->lang : ich->kurz, T("verbunden", "connected"), min(ich->akku, 100));
            else snprintf(z, sizeof z, "%s  ·  %s", ich ? ich->kurz : "?", T("verbunden", "connected"));
            break;
    }
    text(z, 20, ZY + ZH / 2, &FreeSansBold12pt7b, ML_DATUM);
    pille(640, ZY + 2, 140, ZH - 4, lage == L_BEREIT ? T("Trennen", "Disconnect") : T("Verbinden", "Connect"), R_WERKZEUG);
    display.fillRect(20, ZY + ZH + 4, W - 40, 1, SCHWARZ);
}

static void zeichneNachrichten() {
    display.fillRect(NX, NY - 6, NW, NH + 6, WEISS);
    text(T("Nachrichten", "Messages"), NX, NY, &FreeSansBold9pt7b, TL_DATUM);
    if (!nachrN) { text(T("noch keine – eingehende Texte erscheinen hier", "none yet – incoming texts appear here"), NX, NY + 40, &FreeSans9pt7b, TL_DATUM); return; }
    for (int i = 0; i < nachrN; i++) {
        const Nachricht& m = nachr[i];
        int y = NY + 26 + i * 50;
        Knoten* k = knotenFuer(m.von, false);
        char kopf[64], a[24];
        alter(m.um, a, sizeof a);
        const char* kn = m.kanal < kanalN ? kanalName[m.kanal] : "?";
        if (m.eigen) snprintf(kopf, sizeof kopf, "%s  ·  %s  ·  %s", a, T("ich", "me"), kn);
        else if (m.rssi) snprintf(kopf, sizeof kopf, "%s  ·  %s  ·  %s  ·  %d dBm", a, k ? k->kurz : "?", kn, m.rssi);
        else snprintf(kopf, sizeof kopf, "%s  ·  %s  ·  %s", a, k ? k->kurz : "?", kn);
        text(kopf, NX, y, &FreeSans9pt7b, TL_DATUM);
        char t[44]; snprintf(t, sizeof t, "%s%s", m.eigen ? "> " : "", m.text);   // 1 Zeile; lange Texte abgeschnitten
        text(t, NX, y + 18, m.eigen ? &FreeSans12pt7b : &FreeSansBold12pt7b, TL_DATUM);
    }
}

static void zeichneKnoten() {
    display.fillRect(KX, KY - 6, KW2, NH + 6, WEISS);
    display.fillRect(KX - 10, KY - 6, 1, NH, SCHWARZ);
    char z[48];
    snprintf(z, sizeof z, T("Knoten (%d)", "Nodes (%d)"), knotenN);
    text(z, KX, KY, &FreeSansBold9pt7b, TL_DATUM);
    int y = KY + 26;
    for (int i = 0; i < knotenN && y < KY + NH - 30; i++) {
        const Knoten& k = knoten[i];
        bool ich = k.nr == ichNr;
        snprintf(z, sizeof z, "%s%s", k.kurz, ich ? T("  (ich)", "  (me)") : "");
        text(z, KX, y, &FreeSansBold9pt7b, TL_DATUM);
        if (k.lat || k.lon) snprintf(z, sizeof z, "%.4f  %.4f", k.lat / 1e7, k.lon / 1e7);
        else snprintf(z, sizeof z, T("keine Position", "no position"));
        text(z, KX, y + 20, &FreeSans9pt7b, TL_DATUM);
        y += 48;
    }
}

static void zeichneAntworten() {
    display.fillRect(0, AY - 8, W, AH + 16, WEISS);
    for (int i = 0; i < 4; i++) pille(AX[i], AY, AW[i], AH, EN ? ANTWORT_EN[i] : ANTWORT_DE[i], lage == L_BEREIT ? R_HAUPT : R_NEBEN);
    char k[32]; snprintf(k, sizeof k, "%s %s", T("Kanal", "Ch."), sendeKanal < kanalN ? kanalName[sendeKanal] : "–");
    pille(CX, AY, CW, AH, k, R_AUSWAHL);
}

static void zeichneLage() { zeichneZeile(); teil(0, ZY, W, ZH + 6); }

static void betreten() {
    statuszeile(T("13 · Mesh", "13 · Mesh"));
    zeichneZeile();
    zeichneNachrichten();
    zeichneKnoten();
    zeichneAntworten();
}

static void alles() { display.fillRect(0, STATUS_H, W, H - STATUS_H, WEISS); betreten(); teil(0, 0, W, H); }

static void taste(Knopf k) {
    if (k == K_UP) {   // ▲ = verbinden / trennen
        if (lage == L_BEREIT) { trennen(); alles(); }
        else { verbinden(); alles(); }
    } else if (k == K_DOWN && kanalN) {   // ▼ = Sendekanal wechseln
        sendeKanal = (sendeKanal + 1) % kanalN;
        zeichneAntworten(); teil(0, AY - 8, W, AH + 16);
    }
}

static void tipp(int x, int y) {
    if (drin(x, y, 640, ZY, 140, ZH)) { taste(K_UP); return; }
    if (drin(x, y, CX, AY, CW, AH)) { taste(K_DOWN); return; }
    for (int i = 0; i < 4; i++) if (drin(x, y, AX[i], AY, AW[i], AH)) {
        if (lage != L_BEREIT) { tone(PIN_BUZZER, 600, 120); return; }
        pille(AX[i], AY, AW[i], AH, EN ? ANTWORT_EN[i] : ANTWORT_DE[i], R_HAUPT, true);
        teil(AX[i], AY, AW[i], AH);
        bool ok = senden(EN ? ANTWORT_EN[i] : ANTWORT_DE[i]);
        klick();
        if (!ok) tone(PIN_BUZZER, 600, 200);
        zeichneAntworten(); zeichneNachrichten();
        teil(0, NY - 6, W, AY + AH + 8 - (NY - 6));
        return;
    }
}

static void tick() {
    if (getrennt && lage != L_AUS) { getrennt = false; chTo = chFrom = chNum = nullptr; lage = L_FEHLER; snprintf(fehler, sizeof fehler, "%s", T("Verbindung verloren", "connection lost")); alles(); return; }
    if (lage == L_BEREIT && anliegend) { anliegend = false; abholen(); }
    if (neuZeichnen) {
        neuZeichnen = false;
        zeichneZeile(); zeichneNachrichten(); zeichneKnoten(); zeichneAntworten();
        teil(0, ZY, W, AY + AH + 8 - ZY);
    }
}

static bool verbunden() { return lage == L_BEREIT || lage == L_LESE; }
static void verlassen() { trennen(); }   // Handy-App soll wieder rankommen

}  // namespace mesh
