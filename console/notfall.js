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
Box: Powerbank, Lampe, Pfeife, Rettungsdecke, Pflaster, Bargeld
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

  /** Blatt auf eine Leinwand 800 × 480 (gleiche Anordnung wie notfallblatt.py) */
  function zeichnen(c, d, en) {
    const g = c.getContext('2d');
    g.fillStyle = '#fff'; g.fillRect(0, 0, W, H);
    g.fillStyle = '#000'; g.strokeStyle = '#000'; g.textBaseline = 'top';
    g.fillRect(0, 0, W, 56);
    g.fillStyle = '#fff'; g.textBaseline = 'middle';
    g.font = font(30, true); g.fillText(en ? 'EMERGENCY' : 'NOTFALL', 20, 29);
    g.font = font(15); g.fillText((en ? 'as of ' : 'Stand ') + datum(), en ? 250 : 200, 30);
    g.fillStyle = '#000'; g.textBaseline = 'top';
    const links = 20, mitte = 420, rechts = 436;
    let y = 74;
    const marke = (x, yy, s) => { g.font = font(14); g.fillText(s.toUpperCase(), x, yy); };
    const linie = (x0, yy, x1, b = 2) => { g.fillRect(x0, yy, x1 - x0, b); };
    if (d.Name) {
      marke(links, y, en ? 'Name' : 'Name'); y += 20;
      passend(g, d.Name, 32, mitte - links, true); g.fillText(d.Name, links, y); y += 40;
      if (d.Zusatz) { passend(g, d.Zusatz, 16, mitte - links); g.fillText(d.Zusatz, links, y); y += 24; }
      linie(links, y + 4, mitte - 10); y += 14;
    }
    if (d.Kontakt.length) {
      marke(links, y, en ? 'In case of emergency call' : 'Im Notfall anrufen'); y += 24;
      d.Kontakt.slice(0, 3).forEach((k, i) => {
        const [wer, nr = ''] = k.split('|').map((s) => s.trim());
        g.beginPath(); g.arc(links + 15, y + 19, 15, 0, 7); g.fill();
        g.fillStyle = '#fff'; g.font = font(17, true); g.textAlign = 'center'; g.textBaseline = 'middle';
        g.fillText(String(i + 1), links + 15, y + 20);
        g.fillStyle = '#000'; g.textAlign = 'left'; g.textBaseline = 'top';
        passend(g, wer, 18, mitte - links - 54); g.fillText(wer, links + 44, y);
        passend(g, nr, 22, mitte - links - 54, true); g.fillText(nr, links + 44, y + 21);
        y += 54;
      });
      linie(links, y, mitte - 10); y += 10;
    }
    if (d.Treffpunkt) {
      marke(links, y, en ? 'Meeting point' : 'Treffpunkt'); y += 22;
      g.font = font(19);
      for (const z of umbrechen(g, d.Treffpunkt, mitte - links - 10).slice(0, 2)) { g.fillText(z, links, y); y += 24; }
    }
    y = 74;
    if (d.Medizin.length) {
      const h = 56 + 30 * Math.min(d.Medizin.length, 5);
      g.beginPath(); g.roundRect(rechts + 3, y + 3, W - 16 - rechts - 6, h - 6, 15); g.lineWidth = 6; g.stroke();
      g.beginPath(); g.roundRect(rechts + 11, y + 11, W - 26 - rechts - 12, h - 22, 9); g.lineWidth = 2; g.stroke();
      g.font = font(14); g.fillText(en ? 'MEDICAL · SHORT' : 'MEDIZIN · KURZ', rechts + 24, y + 20);
      let yy = y + 42;
      d.Medizin.slice(0, 5).forEach((m, i) => { passend(g, m, 21, W - 26 - rechts - 34, i === 0); g.fillText(m, rechts + 24, yy); yy += 30; });
      y += h + 18;
    }
    if (d.Box) {
      marke(rechts, y, en ? 'In the box' : 'In der Box'); y += 22;
      g.font = font(16);
      for (const z of umbrechen(g, d.Box, W - 16 - rechts).slice(0, 5)) { g.fillText(z, rechts, y); y += 21; }
    }
    if (d.Hinweis) { passend(g, d.Hinweis, 15, W - 16 - rechts); g.fillText(d.Hinweis, rechts, Math.min(y + 6, 392)); }
    linie(0, 415, W);
    for (const [x0, y0, x1, y1, s, a] of KNOEPFE) {
      const text = a === 'spar' && en ? 'Power save' : s;
      g.beginPath(); g.roundRect(x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 21);
      g.textAlign = 'center'; g.textBaseline = 'middle';
      if (a === 'sos') { g.fill(); g.fillStyle = '#fff'; g.font = font(20, true); g.fillText(text, (x0 + x1) / 2, (y0 + y1) / 2 + 1); g.fillStyle = '#000'; }
      else { g.lineWidth = 2; g.stroke(); g.font = font(18); g.fillText(text, (x0 + x1) / 2, (y0 + y1) / 2 + 1); }
      g.textAlign = 'left'; g.textBaseline = 'top';
    }
    g.font = font(14); g.textAlign = 'right'; g.textBaseline = 'middle';
    g.fillText(en ? 'Battery empty: image stays' : 'Akku leer: Bild bleibt stehen', W - 16, 450);
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
    let bereit = antwortAbwarten(3000);           // Handschlag ab FW 0.41.0 („DATEI BEREIT“), ältere legen gleich los
    await roh(new TextEncoder().encode(`DATEI ${rel} ${bytes.length}\n`));
    const b = await bereit;
    if (b.startsWith('DATEI FEHLER')) throw new Error(b);
    const antwort = antwortAbwarten(20000);
    for (let o = 0; o < bytes.length; o += 4096) await roh(bytes.subarray(o, o + 4096));
    const a = await antwort;
    if (!a.startsWith('DATEI OK')) throw new Error(a || tt('keine Antwort – EDC-Firmware ≥ 0.40.0 und SD-Karte nötig', 'no answer – needs EDC firmware ≥ 0.40.0 and an SD card'));
  }
  $n('#nfAufspielen').onclick = async () => {
    neu();
    if (typeof verbunden === 'function' && !verbunden()) { await verbinden(); if (!verbunden()) return; }
    meld.textContent = tt('spielt auf …', 'uploading …');
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
