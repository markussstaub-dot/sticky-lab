// Spickzettel-Konsole (SPEC reterminal-sticky „Spickzettel-Konsole“, 15.11.2026): Karten fürs EDC-Sticky am Zenbook
// pflegen. Die Karten gehören dem Spickzettel-Feld der Sticky-App (App = Gehirn); hier nur Schreibtisch und Vorschau.
// Format wie in der App: „## Titel“, darunter Zeilen, „!“ am Anfang = fett. Dazu Szenen-Marken im Titel
// („## Reifendruck #micro #notfall“), die nur hier leben: Zur App gehen die Karten der gewählten Szene ohne Marken.
// Inhalte sind persönlich: sie liegen nur im Browser-Speicher und in der lokalen Datei, nie im (öffentlichen) Repo.

// ---- Format (ohne DOM, auch für Node-Tests) ----
const SpickFormat = (() => {
  const MARKE = /(^|\s)#([\p{L}\p{N}_-]+)/gu;
  /** Text (Datei, App-Feld oder altes Format „## Karte 1: …“ mit „- “-Zeilen) → { kopf, karten:[{titel, marken, zeilen}] } */
  function lesen(text) {
    text = String(text ?? '').replace(/\r\n?/g, '\n');
    const teile = text.split(/^## /m);
    const kopf = teile.shift().trim();
    const karten = teile.map((block) => {
      const [ersteZeile, ...rest] = block.split('\n');
      const marken = [];
      const titel = ersteZeile.replace(MARKE, (_, vor, m) => { marken.push(m.toLowerCase()); return vor; })
        .replace(/^Karte\s+\d+\s*:\s*/i, '').replace(/\s+/g, ' ').trim();
      const zeilen = rest.map((z) => z.trimEnd().replace(/^\s*[-*]\s+/, '')).filter((z) => z.trim());
      return { titel, marken: [...new Set(marken)], zeilen };
    }).filter((k) => k.titel || k.zeilen.length);
    return { kopf, karten };
  }
  const karteText = (k, mitMarken) =>
    `## ${k.titel || '?'}${mitMarken && k.marken.length ? ' ' + k.marken.map((m) => '#' + m).join(' ') : ''}\n` +
    k.zeilen.join('\n');
  /** Vorrat als Datei: Kopf bleibt stehen, Karten mit Marken */
  function datei(vorrat) {
    return (vorrat.kopf ? vorrat.kopf + '\n\n' : '') + vorrat.karten.map((k) => karteText(k, true)).join('\n\n') + '\n';
  }
  /** Karte gehört zur Szene: Szene „alle“, oder Karte ohne Marke, oder Marke passt */
  const inSzene = (k, szene) => !szene || szene === 'alle' || !k.marken.length || k.marken.includes(szene);
  /** Für das Spickzettel-Feld der App: nur die Karten der Szene, ohne Marken und ohne Kopf */
  function fuerApp(vorrat, szene) {
    return vorrat.karten.filter((k) => inSzene(k, szene)).map((k) => karteText(k, false)).join('\n\n') + '\n';
  }
  /** Aus der App Zurückgeholtes einpflegen: gleicher Titel → Zeilen ersetzen, Marken bleiben; Neues hinten an
   *  (mit der Marke der gewählten Szene). Karten, die nicht kamen, bleiben (alles sammeln). */
  function einpflegen(vorrat, text, szene) {
    let neu = 0, geaendert = 0;
    for (const k of lesen(text).karten) {
      const alt = vorrat.karten.find((a) => a.titel.toLowerCase() === k.titel.toLowerCase());
      if (alt) {
        if (alt.zeilen.join('\n') !== k.zeilen.join('\n')) { alt.zeilen = k.zeilen; geaendert++; }
        for (const m of k.marken) if (!alt.marken.includes(m)) alt.marken.push(m);
      } else {
        if (!k.marken.length && szene && szene !== 'alle') k.marken = [szene];
        vorrat.karten.push(k); neu++;
      }
    }
    return { neu, geaendert };
  }
  return { lesen, datei, fuerApp, einpflegen, inSzene };
})();

// ---- Zeichnen: Abschrift von zeichneTextseite() aus reterminal-sticky/sticky-app/src/bordbuch.js ----
// (bei Änderungen dort hier nachziehen; liefert zusätzlich Schriftgröße und Zeilenzahl für die Hinweise)
const SpickBild = (() => {
  const BREITE = 800, HOEHE = 480, FONT = 'system-ui, sans-serif';
  function kopf(g, titel, rechts) {
    g.fillStyle = '#fff'; g.fillRect(0, 0, BREITE, HOEHE);
    g.fillStyle = '#000'; g.fillRect(0, 0, BREITE, 56);
    g.fillStyle = '#fff'; g.textBaseline = 'middle'; g.textAlign = 'left';
    g.font = `bold 30px ${FONT}`; g.fillText(titel, 18, 29);
    g.textAlign = 'right'; g.font = `16px ${FONT}`; g.fillText(rechts, BREITE - 18, 45);
    g.textAlign = 'left'; g.fillStyle = '#000';
  }
  function textseite(g, seite, fuss) {
    kopf(g, seite.titel, fuss);
    const umbrechen = (t, fett, groesse) => {
      g.font = `${fett ? 'bold ' : ''}${groesse}px ${FONT}`;
      const breit = BREITE - (fett ? 18 : 30) - 16;
      const teile = [];
      let rest = t;
      while (g.measureText(rest).width > breit) {
        let i = rest.length;
        do i = rest.lastIndexOf(' ', i - 1); while (i > 0 && g.measureText(rest.slice(0, i)).width > breit);
        if (i <= 0) break;
        teile.push(rest.slice(0, i).trimEnd());
        rest = '  ' + rest.slice(i).trimStart();
      }
      teile.push(rest);
      return teile;
    };
    const setzen = (groesse) => seite.zeilen.flatMap((z) => {
      const fett = z.startsWith('!');
      return umbrechen(fett ? z.slice(1) : z, fett, groesse).map((t) => ({ t, fett }));
    });
    let groesse = 26, zeilen = setzen(groesse);
    const hoehe = (zs, gr) => zs.reduce((h, z) => h + gr * (z.fett ? 1.38 : 1.28), 72 + gr * 0.6);
    while (groesse > 16 && hoehe(zeilen, groesse) > HOEHE - 6) zeilen = setzen(--groesse);
    let y = 72 + groesse * 0.6;
    for (const { t, fett } of zeilen) {
      g.font = `${fett ? 'bold ' : ''}${groesse}px ${FONT}`;
      g.fillText(t, fett ? 18 : 30, y);
      y += groesse * (fett ? 1.38 : 1.28);
    }
    return { groesse, bildzeilen: zeilen.length, passt: hoehe(zeilen, groesse) <= HOEHE - 6 };
  }
  return { textseite };
})();

if (typeof module !== 'undefined') module.exports = { SpickFormat };

// ---- Oberfläche ----
if (typeof document !== 'undefined' && document.getElementById('spick')) (() => {
  const SZENEN = ['micro', 'werkstatt', 'lab', 'notfall', 'unterwegs'];
  const q = (id) => document.getElementById(id);
  const lies = (k, std) => { try { return localStorage.getItem(k) ?? std; } catch { return std; } };
  const schreib = (k, v) => { try { localStorage.setItem(k, v); } catch { /* egal */ } };
  let vorrat = SpickFormat.lesen(lies('spick.vorrat', ''));
  let szene = lies('spick.szene', 'alle');
  let wahl = Math.min(+lies('spick.wahl', 0) || 0, Math.max(0, vorrat.karten.length - 1));
  let datei = null;            // FileSystemFileHandle der Vorratsdatei (Edge/Chrome)
  let ungespeichert = false;

  // Datei-Griff im Browser merken (IndexedDB), damit „Datei öffnen“ nur einmal nötig ist
  const idb = (fn) => new Promise((ok, nein) => {
    const r = indexedDB.open('spickzettel', 1);
    r.onupgradeneeded = () => r.result.createObjectStore('griffe');
    r.onsuccess = () => { const tx = r.result.transaction('griffe', 'readwrite'); const st = tx.objectStore('griffe'); const a = fn(st); tx.oncomplete = () => ok(a.result); tx.onerror = () => nein(tx.error); };
    r.onerror = () => nein(r.error);
  });

  const meldung = (text, schlecht) => { const m = q('spMeldung'); m.textContent = text; m.style.color = schlecht ? 'var(--bad)' : ''; };
  const sichtbar = () => vorrat.karten.map((k, i) => ({ k, i })).filter(({ k }) => SpickFormat.inSzene(k, szene));

  function merken() {
    schreib('spick.vorrat', SpickFormat.datei(vorrat));
    schreib('spick.szene', szene); schreib('spick.wahl', String(wahl));
  }
  function geaendert() { ungespeichert = true; merken(); dateiZeile(); }

  function dateiZeile() {
    q('spDatei').textContent = (datei ? datei.name : t('keine Datei', 'no file')) + (ungespeichert && datei ? t(' · ungespeichert', ' · unsaved') : '');
  }

  function szenenZeigen() {
    const alle = ['alle', ...new Set([...SZENEN, ...vorrat.karten.flatMap((k) => k.marken)])];
    const box = q('spSzenen'); box.textContent = '';
    for (const s of alle) {
      const b = document.createElement('button');
      b.textContent = s === 'alle' ? t('Alle', 'All') : s[0].toUpperCase() + s.slice(1);
      b.className = s === szene ? 'voll' : '';
      const n = vorrat.karten.filter((k) => SpickFormat.inSzene(k, s)).length;
      b.title = `${n} ${t('Karten', 'cards')}`;
      b.onclick = () => { szene = s; merken(); allesZeigen(); };
      box.append(b);
    }
  }

  function listeZeigen() {
    const ul = q('spListe'); ul.textContent = '';
    const sv = sichtbar();
    if (!sv.length) {
      const li = document.createElement('li'); li.className = 'leer';
      li.textContent = vorrat.karten.length ? t('Keine Karte in dieser Szene.', 'No card in this scene.')
        : t('Noch leer: „Datei öffnen“ (SPICKZETTEL-EDC.md) oder „Aus Zwischenablage holen“.', 'Empty: “Open file” or “Get from clipboard”.');
      ul.append(li);
    }
    for (const { k, i } of sv) {
      const li = document.createElement('li');
      li.className = i === wahl ? 'gewaehlt' : '';
      const b = document.createElement('button'); b.className = 'sptitel';
      b.textContent = k.titel || '?';
      const m = document.createElement('small'); m.textContent = k.marken.length ? k.marken.map((x) => '#' + x).join(' ') : t('überall', 'everywhere');
      b.append(m);
      b.onclick = () => { wahl = i; merken(); allesZeigen(); q('spZeilen').focus(); };
      li.append(b);
      ul.append(li);
    }
    q('spAnzahl').textContent = `${sv.length}/${vorrat.karten.length} ${t('Karten', 'cards')}`;
  }

  function karteZeigen() {
    const k = vorrat.karten[wahl];
    for (const id of ['spTitel', 'spMarken', 'spZeilen', 'spHoch', 'spRunter', 'spLoeschen']) q(id).disabled = !k;
    q('spTitel').value = k?.titel ?? '';
    q('spMarken').value = k ? k.marken.map((m) => '#' + m).join(' ') : '';
    q('spZeilen').value = k ? k.zeilen.join('\n') : '';
    vorschau();
  }

  function vorschau() {
    const c = q('spBild'), g = c.getContext('2d');
    const k = vorrat.karten[wahl];
    if (!k) { g.fillStyle = '#fff'; g.fillRect(0, 0, 800, 480); q('spPruef').textContent = ''; return; }
    const sv = sichtbar(), pos = sv.findIndex((x) => x.i === wahl);
    const fuss = `${t('Spickzettel', 'Cheat sheet')} ${pos + 1}/${sv.length}`;
    const r = SpickBild.textseite(g, k, fuss);
    const hinweise = [`${k.zeilen.length} ${t('Zeilen', 'lines')}`, `${t('Schrift', 'font')} ${r.groesse} px`];
    let schlecht = false;
    if (k.zeilen.length > 10) { hinweise.push(t('mehr als 10 Zeilen – lieber teilen', 'more than 10 lines – better split')); schlecht = true; }
    if (!r.passt) { hinweise.push(t('passt nicht ganz aufs Bild', 'does not fit')); schlecht = true; }
    else if (r.groesse < 20) { hinweise.push(t('Schrift wird klein', 'font gets small')); schlecht = true; }
    if (pos < 0) hinweise.push(t('nicht in dieser Szene', 'not in this scene'));
    q('spPruef').textContent = hinweise.join(' · ');
    q('spPruef').style.color = schlecht ? 'var(--bad)' : '';
  }

  function allesZeigen() { szenenZeigen(); listeZeigen(); karteZeigen(); dateiZeile(); }

  // Eingaben
  q('spTitel').oninput = (e) => { const k = vorrat.karten[wahl]; if (!k) return; k.titel = e.target.value.replace(/\s+/g, ' ').replace(/^#+\s*/, ''); geaendert(); listeZeigen(); vorschau(); };
  q('spMarken').oninput = (e) => { const k = vorrat.karten[wahl]; if (!k) return; k.marken = [...new Set((e.target.value.match(/[\p{L}\p{N}_-]+/gu) || []).map((m) => m.toLowerCase()))]; geaendert(); listeZeigen(); vorschau(); };
  q('spMarken').onchange = () => szenenZeigen();
  q('spZeilen').oninput = (e) => {
    const k = vorrat.karten[wahl]; if (!k) return;
    // „## “ am Zeilenanfang würde in der App eine neue Karte beginnen
    k.zeilen = e.target.value.split('\n').map((z) => z.trimEnd().replace(/^##\s/, '')).filter((z) => z.trim());
    geaendert(); vorschau();
  };

  function neueKarte() {
    vorrat.karten.push({ titel: t('Neue Karte', 'New card'), marken: szene === 'alle' ? [] : [szene], zeilen: [] });
    wahl = vorrat.karten.length - 1; geaendert(); allesZeigen();
    q('spTitel').focus(); q('spTitel').select();
  }
  function schieben(d) {
    // innerhalb der sichtbaren Karten tauschen (Szene gefiltert), im Vorrat an deren Plätze
    const sv = sichtbar(), pos = sv.findIndex((x) => x.i === wahl), ziel = sv[pos + d];
    if (pos < 0 || !ziel) return;
    const a = vorrat.karten;
    [a[wahl], a[ziel.i]] = [a[ziel.i], a[wahl]];
    wahl = ziel.i; geaendert(); listeZeigen(); vorschau();
  }
  q('spNeu').onclick = neueKarte;
  q('spHoch').onclick = () => schieben(-1);
  q('spRunter').onclick = () => schieben(1);
  q('spLoeschen').onclick = () => {
    const k = vorrat.karten[wahl]; if (!k) return;
    if (!confirm(t(`Karte „${k.titel}“ löschen?`, `Delete card “${k.titel}”?`))) return;
    vorrat.karten.splice(wahl, 1); wahl = Math.max(0, wahl - 1); geaendert(); allesZeigen();
  };

  // Datei: einmal öffnen, danach zurückschreiben (File System Access in Edge/Chrome); sonst herunterladen
  async function dateiLesen(griff) {
    const text = await (await griff.getFile()).text();
    vorrat = SpickFormat.lesen(text); wahl = 0; datei = griff; ungespeichert = false;
    merken(); allesZeigen();
    meldung(t(`${vorrat.karten.length} Karten aus ${griff.name}`, `${vorrat.karten.length} cards from ${griff.name}`));
  }
  q('spOeffnen').onclick = async () => {
    if (ungespeichert && datei && !confirm(t('Ungespeicherte Änderungen verwerfen?', 'Discard unsaved changes?'))) return;
    try {
      if ('showOpenFilePicker' in window) {
        const [griff] = await showOpenFilePicker({ types: [{ description: 'Spickzettel', accept: { 'text/markdown': ['.md', '.txt'] } }] });
        await dateiLesen(griff);
        idb((st) => st.put(griff, 'vorrat')).catch(() => {});
      } else q('spWahl').click();
    } catch (e) { if (e.name !== 'AbortError') meldung(e.message, true); }
  };
  q('spWahl').onchange = async (e) => {
    const f = e.target.files[0]; if (!f) return;
    vorrat = SpickFormat.lesen(await f.text()); wahl = 0; merken(); allesZeigen();
    meldung(t(`${vorrat.karten.length} Karten gelesen (Speichern lädt herunter)`, `${vorrat.karten.length} cards read (Save downloads)`));
  };
  async function speichern() {
    const text = SpickFormat.datei(vorrat);
    try {
      if (datei) {
        if ((await datei.queryPermission?.({ mode: 'readwrite' })) !== 'granted') await datei.requestPermission?.({ mode: 'readwrite' });
        const w = await datei.createWritable(); await w.write(text); await w.close();
        ungespeichert = false; dateiZeile(); meldung(t(`Gespeichert in ${datei.name}`, `Saved to ${datei.name}`));
      } else {
        const a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([text], { type: 'text/markdown' }));
        a.download = 'SPICKZETTEL-EDC.md'; a.click();
        meldung(t('Heruntergeladen (keine Datei geöffnet)', 'Downloaded (no file open)'));
      }
    } catch (e) { meldung(e.message, true); }
  }
  q('spSpeichern').onclick = speichern;

  // Zwischenablage: hin im App-Format der Szene, zurück mit Einpflegen
  q('spKopieren').onclick = async () => {
    const n = vorrat.karten.filter((k) => SpickFormat.inSzene(k, szene)).length;
    try {
      await navigator.clipboard.writeText(SpickFormat.fuerApp(vorrat, szene));
      meldung(t(`${n} Karten kopiert – am Fold: Bordbuch → Spickzettel, alles markieren, Einfügen, Aufspielen`,
        `${n} cards copied – on the Fold: Logbook → Cheat sheets, select all, paste, upload`));
    } catch (e) { meldung(e.message, true); }
  };
  q('spHolen').onclick = async () => {
    try {
      const text = await navigator.clipboard.readText();
      if (!/^## /m.test(text)) { meldung(t('In der Zwischenablage stehen keine Karten („## Titel“).', 'No cards (“## Title”) in the clipboard.'), true); return; }
      const r = SpickFormat.einpflegen(vorrat, text, szene);
      geaendert(); allesZeigen();
      meldung(t(`${r.neu} neu, ${r.geaendert} geändert, Rest bleibt`, `${r.neu} new, ${r.geaendert} changed, rest kept`));
    } catch (e) { meldung(e.message, true); }
  };

  // Tastatur: Strg+S speichern, Strg+Enter neue Karte, Alt+↑/↓ sortieren, Alt+Bild↑/↓ andere Karte
  document.addEventListener('keydown', (e) => {
    if (q('spick').hidden) return;
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') { e.preventDefault(); speichern(); }
    else if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') { e.preventDefault(); neueKarte(); }
    else if (e.altKey && (e.key === 'ArrowUp' || e.key === 'ArrowDown')) { e.preventDefault(); schieben(e.key === 'ArrowUp' ? -1 : 1); }
    else if (e.altKey && (e.key === 'PageUp' || e.key === 'PageDown')) {
      e.preventDefault();
      const sv = sichtbar(), pos = sv.findIndex((x) => x.i === wahl), z = sv[pos + (e.key === 'PageUp' ? -1 : 1)];
      if (z) { wahl = z.i; merken(); allesZeigen(); }
    }
  });
  addEventListener('beforeunload', (e) => { if (ungespeichert && datei) e.preventDefault(); });

  // Beim Laden: zuletzt geöffnete Datei wieder anbieten (Erlaubnis braucht einen Klick)
  idb((st) => st.get('vorrat')).then((griff) => {
    if (!griff) return;
    datei = griff; dateiZeile();
    q('spWieder').hidden = false;
    q('spWieder').textContent = t(`${griff.name} neu einlesen`, `Reload ${griff.name}`);
    q('spWieder').onclick = async () => {
      try {
        if ((await griff.requestPermission({ mode: 'readwrite' })) !== 'granted') return;
        await dateiLesen(griff); q('spWieder').hidden = true;
      } catch (e) { meldung(e.message, true); }
    };
  }).catch(() => {});

  window.spickZeigen = () => vorschau();
  allesZeigen();
})();
