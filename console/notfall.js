// Notfallblatt (SPEC reterminal-sticky „SD-Karte als Bibliothek und Bugout-Sticky“, 17.11.2026): allgemeine Fassung für
// alle mit EDC-Firmware ≥ 0.40.0, ohne Python. Gleiches Format und gleiches Bild wie sticky-app/tools/notfallblatt.py:
// Zeilen „Schlüssel: Wert“ (Name, Zusatz, Kontakt: Wer | Nummer, Medizin, Treffpunkt, Box, Hinweis).
// Die Daten bleiben im Browser (localStorage) bzw. in der Datei, die man selbst speichert – nie im Repo, nie im Netz.
// Aufspielen per USB-Kabel (Web Serial / WebUSB der Console): Zeile „DATEI <pfad> <länge>“ + Bytes, Antwort „DATEI OK …“.
// Ohne Kabel: zwei Dateien herunterladen und auf die SD-Karte nach /soll/widgets/notfall/ kopieren.

const Notfall = (() => {
  const W = 800, H = 480, SCHRIFT = '"Helvetica Neue", Helvetica, Arial, sans-serif';
  const MUSTER = `# Notfallblatt – Zeilen „Schlüssel: Wert“, was fehlt, bleibt weg. Zeilen mit # werden übersprungen.
# Das Blatt ist offen lesbar (wer den Sticky findet). Medizin nur das Lebenswichtige.
Name: Max Mustermann
Zusatz: geb. 01.01.1970 · spricht Deutsch, Englisch
Kontakt: Erika (Partnerin) | +49 170 0000000
Kontakt: Paul (Bruder) | +49 151 0000000
Medizin: Blutgruppe A+
Treffpunkt: Bahnhof, Haupteingang
Box: Powerbank, Lesebrille, Lampe, Pfeife, Rettungsdecke, Pflaster, Bargeld
Hinweis: Details auf der Papierkarte
`;
  const MUSTER_EN = `# Emergency sheet – lines "Key: value", missing keys are left out. Lines starting with # are skipped.
# The sheet can be read by anyone who finds the Sticky. Medical: only what is vital.
Name: Jane Doe
Zusatz: born 1970-01-01 · speaks English, German
Kontakt: John (partner) | +1 555 0100
Kontakt: Mary (sister) | +1 555 0101
Medizin: Blood type 0-
Treffpunkt: Station, main entrance
Box: power bank, torch, whistle, foil blanket, plasters, cash
Hinweis: details on the paper card
`;

  /** Text → { Name, Zusatz, Kontakt: [], Medizin: [], … } (Schlüssel auch englisch) */
  function lesen(text) {
    const ALIAS = { contact: 'Kontakt', medical: 'Medizin', meeting: 'Treffpunkt', 'meeting point': 'Treffpunkt', note: 'Hinweis', extra: 'Zusatz' };
    const d = { Kontakt: [], Medizin: [] };
    for (let z of String(text).split(/\r?\n/)) {
      z = z.trim();
      if (!z || z.startsWith('#') || !z.includes(':')) continue;
      const i = z.indexOf(':');
      let k = z.slice(0, i).trim(); const v = z.slice(i + 1).trim();
      k = ALIAS[k.toLowerCase()] ?? k.charAt(0).toUpperCase() + k.slice(1).toLowerCase();
      if (Array.isArray(d[k])) d[k].push(v); else d[k] = v;
    }
    return d;
  }

  const font = (gr, fett = false) => `${fett ? 700 : 400} ${gr}px ${SCHRIFT}`;
  function passend(g, text, gr, breite, fett = false, min = 12) {
    while (gr > min) { g.font = font(gr, fett); if (g.measureText(text).width <= breite) break; gr--; }
    g.font = font(gr, fett);
  }
  function umbrechen(g, text, breite) {
    const zeilen = []; let z = '';
    for (const w of text.replace(/,/g, ', ').split(/\s+/).filter(Boolean)) {
      const t2 = (z + ' ' + w).trim();
      if (g.measureText(t2).width <= breite) z = t2; else { if (z) zeilen.push(z); z = w; }
    }
    if (z) zeilen.push(z);
    return zeilen;
  }
  const datum = () => new Date().toLocaleDateString('de-DE', { day: '2-digit', month: '2-digit', year: 'numeric' });
  const KNOEPFE = [[20, 428, 220, 472, 'SOS', 'sos'], [236, 428, 436, 472, 'Sparmodus', 'spar']];

  /** Blatt auf eine Leinwand 800 × 480 (gleiche Anordnung wie notfallblatt.py; nichts unter 22 px, liest meist ein Helfer) */
  function zeichnen(c, d, en) {
    const G = 22, unten = 404;
    const g = c.getContext('2d');
    g.fillStyle = '#fff'; g.fillRect(0, 0, W, H);
    g.fillStyle = '#000'; g.strokeStyle = '#000'; g.textBaseline = 'top';
    g.fillRect(0, 0, W, 56);
    g.fillStyle = '#fff'; g.textBaseline = 'middle';
    g.font = font(30, true); g.fillText(en ? 'EMERGENCY' : 'NOTFALL', 20, 29);
    g.font = font(G); g.fillText((en ? 'as of ' : 'Stand ') + datum(), en ? 240 : 190, 30);
    g.fillStyle = '#000'; g.textBaseline = 'top';
    const links = 20, mitte = 420, rechts = 436;
    let y = 66;
    const marke = (x, yy, s) => { g.font = font(G); g.fillText(s, x, yy); };
    const linie = (x0, yy, x1, b = 2) => { g.fillRect(x0, yy, x1 - x0, b); };
    if (d.Name) {
      passend(g, d.Name, 34, mitte - links, true, G); g.fillText(d.Name, links, y); y += 42;
      if (d.Zusatz) { g.font = font(G); for (const z of umbrechen(g, d.Zusatz, mitte - links - 10).slice(0, 2)) { g.fillText(z, links, y); y += 28; } }
      linie(links, y + 4, mitte - 10); y += 14;
    }
    if (d.Kontakt.length) {
      marke(links, y, en ? 'In case of emergency call' : 'Im Notfall anrufen'); y += 30;
      d.Kontakt.slice(0, d.Treffpunkt ? 2 : 3).forEach((k, i) => {
        const [wer, nr = ''] = k.split('|').map((s) => s.trim());
        g.beginPath(); g.arc(links + 17, y + 25, 17, 0, 7); g.fill();
        g.fillStyle = '#fff'; g.font = font(G, true); g.textAlign = 'center'; g.textBaseline = 'middle';
        g.fillText(String(i + 1), links + 17, y + 26);
        g.fillStyle = '#000'; g.textAlign = 'left'; g.textBaseline = 'top';
        passend(g, wer, G, mitte - links - 56, false, G); g.fillText(wer, links + 46, y);
        passend(g, nr, 28, mitte - links - 56, true, G); g.fillText(nr, links + 46, y + 25);
        y += 62;
      });
      linie(links, y, mitte - 10); y += 10;
    }
    if (d.Treffpunkt) {
      marke(links, y, en ? 'Meeting point' : 'Treffpunkt'); y += 28;
      g.font = font(G, true);
      for (const z of umbrechen(g, d.Treffpunkt, mitte - links - 10)) { if (y + G > unten) break; g.fillText(z, links, y); y += 28; }
    }
    y = 66;
    if (d.Medizin.length) {
      const n = Math.min(d.Medizin.length, 4), h = 60 + 32 * n;
      g.beginPath(); g.roundRect(rechts + 3, y + 3, W - 16 - rechts - 6, h - 6, 15); g.lineWidth = 6; g.stroke();
      g.beginPath(); g.roundRect(rechts + 11, y + 11, W - 26 - rechts - 12, h - 22, 9); g.lineWidth = 2; g.stroke();
      g.font = font(G); g.fillText(en ? 'Medical' : 'Medizin', rechts + 24, y + 18);
      let yy = y + 46;
      d.Medizin.slice(0, n).forEach((m, i) => { passend(g, m, 24, W - 26 - rechts - 34, i === 0, G); g.fillText(m, rechts + 24, yy); yy += 32; });
      y += h + 12;
    }
    g.font = font(G);
    const platz = Math.floor((unten - y) / 27);
    let hinweis = d.Hinweis ? umbrechen(g, d.Hinweis, W - 16 - rechts) : [];
    let box = d.Box ? [en ? 'In the box' : 'In der Box', ...umbrechen(g, d.Box, W - 16 - rechts)] : [];
    if (box.length + hinweis.length > platz) {          // zu lang: Hinweis ganz oder gar nicht, Box-Liste kürzen
      if (hinweis.length > Math.floor(platz / 2)) hinweis = [];
      box = box.slice(0, Math.max(0, platz - hinweis.length));
      if (box.length > 1) box[box.length - 1] = box[box.length - 1].replace(/,$/, '') + ' …';
    }
    for (const z of [...box, ...hinweis]) { g.fillText(z, rechts, y); y += 27; }
    linie(0, 415, W);
    for (const [x0, y0, x1, y1, s, a] of KNOEPFE) {
      const text = a === 'spar' && en ? 'Power save' : s;
      g.beginPath(); g.roundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 21);
      g.textAlign = 'center'; g.textBaseline = 'middle';
      if (a === 'sos') { g.fill(); g.fillStyle = '#fff'; g.font = font(26, true); g.fillText(text, (x0 + x1) / 2, (y0 + y1) / 2 + 1); g.fillStyle = '#000'; }
      else { g.lineWidth = 2; g.stroke(); g.font = font(G); g.fillText(text, (x0 + x1) / 2, (y0 + y1) / 2 + 1); }
      g.textAlign = 'left'; g.textBaseline = 'top';
    }
    g.font = font(G); g.textAlign = 'right'; g.textBaseline = 'middle';
    g.fillText(en ? 'Battery empty: stays' : 'Akku leer: bleibt stehen', W - 16, 450);
    g.textAlign = 'left'; g.textBaseline = 'top';
    // auf reines Schwarz/Weiß bringen (wie das Display), damit die Vorschau ehrlich ist
    const img = g.getImageData(0, 0, W, H);
    for (let i = 0; i < img.data.length; i += 4) { const v = img.data[i] < 128 ? 0 : 255; img.data[i] = img.data[i + 1] = img.data[i + 2] = v; img.data[i + 3] = 255; }
    g.putImageData(img, 0, 0);
  }

  /** Leinwand → 1 bpp, MSB zuerst, 1 = schwarz (wie imgBuf im Sticky) */
  function bits(c) {
    const p = c.getContext('2d').getImageData(0, 0, W, H).data, out = new Uint8Array(W * H / 8);
    for (let i = 0; i < W * H; i++) if (p[i * 4] < 128) out[i >> 3] |= 0x80 >> (i & 7);
    return out;
  }
  const felder = () => 'typ notfall\n' + KNOEPFE.map(([x0, y0, x1, y1, , a]) => `knopf ${x0 - 8} ${y0 - 8} ${x1 + 8} ${Math.min(H, y1 + 8)} ${a}\n`).join('');

  /** Papierkarte: Scheckkarte 85,6 × 54 mm, vorn und hinten, 600 dpi → zwei Leinwände */
  function karte(d, en) {
    const MM = 600 / 25.4, PT = 600 / 72, KB = Math.round(85.6 * MM), KH = Math.round(54 * MM), rand = Math.round(3 * MM), breite = KB - 2 * rand;
    const neu = () => { const c = document.createElement('canvas'); c.width = KB; c.height = KH; const g = c.getContext('2d'); g.fillStyle = '#fff'; g.fillRect(0, 0, KB, KH); g.fillStyle = '#000'; g.strokeStyle = '#000'; g.textBaseline = 'top'; return [c, g]; };
    const [vorn, g] = neu();
    const kopf = Math.round(9 * MM);
    g.fillRect(0, 0, KB, kopf);
    g.fillStyle = '#fff'; g.textBaseline = 'middle'; g.font = font(Math.round(11 * PT), true);
    const titel = en ? 'EMERGENCY' : 'NOTFALL';
    g.fillText(titel, rand, kopf / 2);
    if (d.Name) { const x = rand + g.measureText(titel).width + 3 * MM; passend(g, d.Name, Math.round(10 * PT), KB - rand - x); g.fillText(d.Name, x, kopf / 2); }
    g.fillStyle = '#000'; g.textBaseline = 'top';
    let y = kopf + 2 * MM;
    if (d.Zusatz) { passend(g, d.Zusatz, Math.round(6.5 * PT), breite); g.fillText(d.Zusatz, rand, y); y += 4 * MM; }
    if (d.Kontakt.length) {
      g.font = font(Math.round(5.5 * PT)); g.fillText(en ? 'IN CASE OF EMERGENCY CALL' : 'IM NOTFALL ANRUFEN', rand, y); y += 3 * MM;
      for (const k of d.Kontakt.slice(0, 3)) {
        const [wer, nr = ''] = k.split('|').map((s) => s.trim());
        g.font = font(Math.round(8 * PT), true); g.fillText(nr, rand, y);
        const x = rand + g.measureText(nr).width + 2 * MM;
        passend(g, wer, Math.round(7 * PT), KB - rand - x); g.fillText(wer, x, y + 0.6 * MM);
        y += 4.2 * MM;
      }
    }
    if (d.Medizin.length) {
      y += 0.8 * MM;
      const unten = KH - rand;
      g.beginPath(); g.roundRect(rand, y, breite, unten - y, 2 * MM); g.lineWidth = 0.5 * MM; g.stroke();
      const zeilen = d.Medizin.slice(0, 5);
      let gr = Math.round(8.5 * PT);
      while (zeilen.length * gr * 1.2 > unten - y - 5 * MM && gr > 5 * PT) gr -= 4;
      let yy = y + 1.5 * MM;
      g.font = font(Math.round(5 * PT)); g.fillText(en ? 'MEDICAL' : 'MEDIZIN', rand + 2 * MM, yy); yy += 2.8 * MM;
      zeilen.forEach((z, i) => { passend(g, z, gr, breite - 4 * MM, i === 0); g.fillText(z, rand + 2 * MM, yy); yy += gr * 1.2; });
    }
    const [hinten, h] = neu();
    y = rand;
    for (const [titel2, wert, gr] of [[en ? 'MEETING POINT' : 'TREFFPUNKT', d.Treffpunkt, 8], [en ? 'NOTE' : 'HINWEIS', d.Hinweis, 7], [en ? 'IN THE BOX' : 'IN DER BOX', d.Box, 6.5]]) {
      if (!wert) continue;
      h.font = font(Math.round(5.5 * PT)); h.fillText(titel2, rand, y); y += 3 * MM;
      h.font = font(Math.round(gr * PT));
      for (const z of umbrechen(h, wert, breite).slice(0, 4)) { h.fillText(z, rand, y); y += gr * PT * 1.2; }
      y += 1.5 * MM;
    }
    h.fillRect(rand, KH - rand - 4 * MM, breite, 0.25 * MM);
    h.font = font(Math.round(5 * PT)); h.textBaseline = 'alphabetic';
    h.fillText((en ? 'As of ' : 'Stand ') + datum() + (en ? ' · same data as on the Sticky (Emergency page)' : ' · gleiche Daten wie am Sticky (Seite Notfall)'), rand, KH - rand);
    return [vorn, hinten];
  }

  return { W, H, MUSTER, MUSTER_EN, lesen, zeichnen, bits, felder, karte };
})();

// ---- Oberfläche ----
(function () {
  if (typeof document === 'undefined') return;
  const SCHLUESSEL = 'sticky.notfall.text';
  const $n = (s) => document.querySelector(s);
  const en = () => typeof SPRACHE !== 'undefined' && SPRACHE === 'en';
  const tt = (de, e) => (en() ? e : de);
  const feld = $n('#nfText'), bild = $n('#nfBild'), meld = $n('#nfMeldung');
  if (!feld) return;
  let text = '';
  try { text = localStorage.getItem(SCHLUESSEL) ?? ''; } catch { /* privat: dann eben leer */ }
  feld.value = text || (en() ? Notfall.MUSTER_EN : Notfall.MUSTER);
  const daten = () => Notfall.lesen(feld.value);
  const neu = () => { Notfall.zeichnen(bild, daten(), en()); try { localStorage.setItem(SCHLUESSEL, feld.value); } catch { /* egal */ } };
  let z = 0;
  feld.addEventListener('input', () => { clearTimeout(z); z = setTimeout(neu, 250); });
  window.notfallZeigen = neu;

  // Antworten des Sticky („DATEI OK …“ / „DATEI FEHLER …“) kommen über zeile() der Console
  let warte = null;
  window.notfallAntwort = (s) => { if (warte) { warte(s); warte = null; } };
  async function roh(bytes) {
    if (typeof usb !== 'undefined' && usb) return usb.dev.transferOut(usb.aus, bytes);
    if (typeof writer !== 'undefined' && writer) return writer.write(bytes);
    throw new Error(tt('nicht verbunden', 'not connected'));
  }
  const antwortAbwarten = (ms) => new Promise((r) => { warte = r; setTimeout(() => r(''), ms); });
  async function datei(rel, bytes) {
    const bereit = antwortAbwarten(20000);        // Handschlag ab FW 0.41.0: erst nach „DATEI BEREIT“ senden
    await roh(new TextEncoder().encode(`DATEI ${rel} ${bytes.length}\n`));
    const b = await bereit;
    if (b.startsWith('DATEI FEHLER')) throw new Error(b);
    if (!b.startsWith('DATEI BEREIT')) throw new Error(tt('kein „bereit“ – EDC-Firmware ≥ 0.41.0 nötig', 'no “ready” – needs EDC firmware ≥ 0.41.0'));
    const antwort = antwortAbwarten(20000);
    for (let o = 0; o < bytes.length; o += 4096) await roh(bytes.subarray(o, o + 4096));
    const a = await antwort;
    if (!a.startsWith('DATEI OK')) throw new Error(a || tt('keine Antwort – EDC-Firmware ≥ 0.40.0 und SD-Karte nötig', 'no answer – needs EDC firmware ≥ 0.40.0 and an SD card'));
  }
  $n('#nfAufspielen').onclick = async () => {
    neu();
    if (typeof verbunden === 'function' && !verbunden()) { await verbinden(); if (!verbunden()) return; }
    const w = antwortAbwarten(3000);              // „Wer hängt am Kabel?“ (FW ≥ 0.42.0)
    await roh(new TextEncoder().encode('WER
'));
    const wer = (await w).split(' ');
    const geraet = wer[0] === 'WER' ? `${wer[1]} · FW ${wer[4]}` : tt('Sticky', 'Sticky');
    let meins = null;
    try { meins = localStorage.getItem('sticky.notfall.geraet'); } catch { /* egal */ }
    // anderes Gerät mit fremdem Blatt: nachfragen; das eigene (zuletzt benutzte) Gerät still aktualisieren
    if (wer[0] === 'WER' && meins && meins !== wer[2] && wer[6] === '1'
        && !confirm(tt(`${wer[1]} ist nicht dein bisheriger Sticky und trägt schon ein Notfallblatt. Überschreiben?`,
                       `${wer[1]} is not your usual Sticky and already holds an emergency sheet. Overwrite?`))) return;
    if (wer[0] === 'WER') try { localStorage.setItem('sticky.notfall.geraet', wer[2]); } catch { /* egal */ }
    meld.textContent = tt(`spielt auf ${geraet} …`, `uploading to ${geraet} …`);
    try {
      await datei('widgets/notfall/bild.bin', Notfall.bits(bild));
      await datei('widgets/notfall/felder.txt', new TextEncoder().encode(Notfall.felder()));
      meld.textContent = tt('Fertig. Am Sticky mit ▲/▼ zur Seite „Notfall“ blättern.', 'Done. On the Sticky, page with ▲/▼ to “Emergency”.');
    } catch (e) { meld.textContent = e.message; }
  };
  const laden = (name, inhalt, typ = 'application/octet-stream') => {
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([inhalt], { type: typ })); a.download = name; a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 2000);
  };
  $n('#nfSd').onclick = () => {
    neu();
    laden('bild.bin', Notfall.bits(bild));
    setTimeout(() => laden('felder.txt', Notfall.felder(), 'text/plain'), 300);
    meld.textContent = tt('Beide Dateien auf die SD-Karte nach /soll/widgets/notfall/ kopieren.', 'Copy both files onto the SD card into /soll/widgets/notfall/.');
  };
  $n('#nfSpeichern').onclick = () => laden('notfall.md', feld.value, 'text/markdown');
  $n('#nfOeffnen').onclick = () => $n('#nfWahl').click();
  $n('#nfWahl').onchange = async (e) => { const f = e.target.files[0]; if (f) { feld.value = await f.text(); neu(); } e.target.value = ''; };
  $n('#nfKarte').onclick = () => {
    const [v, h] = Notfall.karte(daten(), en());
    const w = window.open('', '_blank');
    if (!w) { meld.textContent = tt('Pop-up blockiert', 'Pop-up blocked'); return; }
    const karte = `<div class="k"><img src="${v.toDataURL()}"><img src="${h.toDataURL()}"></div>`;
    w.document.write(`<!doctype html><meta charset="utf-8"><title>${tt('Notfallkarte', 'Emergency card')}</title>
<style>@page{size:A4;margin:20mm}body{font:12px Arial,sans-serif;margin:0}.k{display:flex;margin:0 0 20mm;width:171.2mm;outline:0.3mm solid #000}
.k img{width:85.6mm;height:54mm}.k img+img{border-left:0.3mm dashed #000}p{margin:0 0 8mm}</style>
<p>${tt('In 100 % drucken, außen ausschneiden, an der gestrichelten Linie falten, laminieren.', 'Print at 100 %, cut along the outside, fold at the dashed line, laminate.')}</p>${karte}${karte}
<script>onload=()=>print()<\/script>`);
    w.document.close();
  };
  neu();
})();
