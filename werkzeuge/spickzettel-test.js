// Prüft das Kartenformat der Spickzettel-Konsole: node werkzeuge/spickzettel-test.js
const assert = require('assert');
const { SpickFormat: F } = require('../console/spickzettel.js');

// altes Format der lokalen Datei: Kopf, „## Karte 1: …“, „- “-Zeilen
const alt = F.lesen('# Kopf\nText davor\n\n## Karte 1: Lab bedienen\n- KI kurz\n- ▲▼ blättern\n\n## Karte 2: Strom #lab\n!Fett\n- zwei\n');
assert.strictEqual(alt.kopf, '# Kopf\nText davor');
assert.deepStrictEqual(alt.karten.map((k) => k.titel), ['Lab bedienen', 'Strom']);
assert.deepStrictEqual(alt.karten[0].zeilen, ['KI kurz', '▲▼ blättern']);
assert.deepStrictEqual(alt.karten[1].marken, ['lab']);
assert.deepStrictEqual(alt.karten[1].zeilen, ['!Fett', 'zwei']);

// Datei rundum: Kopf und Marken bleiben
const v = F.lesen('Kopf\n\n## Reifen #micro #Notfall\n!Druck\n2,0 bar\n\n## Überall\neins\n\n## Drucker #werkstatt\nDüse\n');
assert.deepStrictEqual(F.lesen(F.datei(v)), v);
assert.match(F.datei(v), /^Kopf\n\n## Reifen #micro #notfall\n/);

// für die App: nur die Szene (plus Karten ohne Marke), ohne Marken und ohne Kopf
assert.strictEqual(F.fuerApp(v, 'micro'), '## Reifen\n!Druck\n2,0 bar\n\n## Überall\neins\n');
assert.strictEqual(F.fuerApp(v, 'alle').match(/^## /gm).length, 3);
assert.strictEqual(/ #[a-z]/.test(F.fuerApp(v, 'notfall')), false);

// App-Feld zurückholen: gleicher Titel → Zeilen neu, Marken bleiben; Neues bekommt die Szene; Rest bleibt
const r = F.einpflegen(v, '## reifen\n!Druck\n2,2 bar\n\n## Pannenhilfe\nNummer\n', 'micro');
assert.deepStrictEqual(r, { neu: 1, geaendert: 1 });
assert.deepStrictEqual(v.karten[0], { titel: 'Reifen', marken: ['micro', 'notfall'], zeilen: ['!Druck', '2,2 bar'] });
assert.deepStrictEqual(v.karten[3], { titel: 'Pannenhilfe', marken: ['micro'], zeilen: ['Nummer'] });
assert.strictEqual(v.karten.length, 4);

// Windows-Zeilenenden und App-Standard (ohne Kopf)
assert.strictEqual(F.lesen('## A\r\neins\r\n').karten[0].zeilen[0], 'eins');
assert.strictEqual(F.lesen('## A\neins').kopf, '');
console.log('spickzettel-test: alles grün');
