"""Messwerte aus der C3-Basis (c3_nachliefern, Rolle BASIS) holen und mit echter Uhrzeit als CSV ablegen.

Aufruf:  python fuehler/c3_auswerten.py [--aus <datei.csv>] [--leeren]
Sucht die Basis am USB (MAC aus c3_nachliefern/geraete.h), fragt ZEIT und DUMP ab und rechnet um:
  Messzeit = jetzt − (Basis-Laufzeit jetzt − Basis-Laufzeit beim Empfang) − „vor“ beim Senden.
Stimmt nur, solange die Basis seit dem Empfang nicht neu gestartet ist (sonst sind die Zeiten verschoben).
"""
import argparse, csv, datetime, os, sys, time
import serial
from serial.tools import list_ports

def basis_mac():
    """MAC der Basis aus c3_nachliefern/geraete.h (nicht im Repo), z. B. {0x12, 0x34, …} → '12:34:…'"""
    import re
    pfad = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'c3_nachliefern', 'geraete.h')
    try:
        teile = re.findall(r'0x([0-9A-Fa-f]{2})', open(pfad, encoding='utf-8').read())
    except OSError:
        sys.exit('c3_nachliefern/geraete.h fehlt (Vorlage: geraete.h.vorlage).')
    return ':'.join(t.upper() for t in teile[:6])


BASIS = None


def frage(s, befehl, ende, wartezeit=10):
    s.reset_input_buffer()
    s.write((befehl + '\n').encode())
    puffer, t = b'', time.time()
    while time.time() - t < wartezeit:
        puffer += s.read(4096)
        if ende.encode() in puffer:
            break
    return puffer.decode('utf-8', 'replace')


def main():
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except AttributeError:
        pass
    ap = argparse.ArgumentParser()
    ap.add_argument('--aus', default=os.path.join(os.path.expanduser('~'), 'OneDrive', 'Desktop', 'Claude', 'privat',
                                                  'c3-messwerte.csv'))
    ap.add_argument('--leeren', action='store_true', help='danach den Speicher der Basis leeren')
    ap.add_argument('--uhr', action='store_true', help='nur die Uhr der Basis stellen (vor der Nacht)')
    a = ap.parse_args()
    basis = basis_mac()
    port = next((p.device for p in list_ports.comports() if (p.serial_number or '').upper() == basis), None)
    if not port:
        sys.exit('Basis nicht am USB gefunden.')
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.3
    s.dtr = s.rts = False                                          # sonst startet der C3 beim Öffnen neu → Zeiten falsch
    s.open()
    time.sleep(1)
    if a.uhr:
        print(frage(s, f'UHR {int(time.time())}', 'UHR ok').strip())
        s.close()
        return
    jetzt = datetime.datetime.now()
    zeit = frage(s, 'ZEIT', 'basis_lauf_s')
    lauf = int(zeit.split('basis_lauf_s')[1].split()[0])
    dump = frage(s, 'DUMP', 'DUMP-ENDE', 30)
    zeilen = dump.split('DUMP-ANFANG', 1)[-1].split('DUMP-ENDE')[0].strip().splitlines()[1:]
    werte = []
    for z in zeilen:
        t = z.strip().split(',')
        if len(t) not in (6, 7):
            continue
        wer, nr, vor, temp10, rssi, rx = t[0], int(t[1]), int(t[2]), int(t[3]), int(t[4]), int(t[5])
        unix = int(t[6]) if len(t) == 7 else 0
        # mit gestellter Uhr: echte Zeit aus dem Flash; sonst zurückrechnen (falsch, wenn die Basis neu gestartet ist)
        wann = datetime.datetime.fromtimestamp(unix) if unix else jetzt - datetime.timedelta(seconds=(lauf - rx) + vor)
        werte.append((wann.strftime('%Y-%m-%d %H:%M:%S'), wer, nr, temp10 / 10, rssi))
    werte.sort()
    os.makedirs(os.path.dirname(a.aus), exist_ok=True)
    with open(a.aus, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f, delimiter=';')
        w.writerow(['zeit', 'fuehler', 'nr', 'temp_c', 'rssi_dbm'])
        w.writerows(werte)
    print(f'{len(werte)} Werte → {a.aus}')
    if werte:
        print(f'erster {werte[0][0]}, letzter {werte[-1][0]}; Lücken in nr zeigen fehlenden Funkkontakt nicht – '
              f'die Werte wurden ja nachgeliefert')
    if a.leeren:
        print(frage(s, 'LEER', 'LEER ok').strip())
    s.close()


if __name__ == '__main__':
    main()
