"""Bildschirmfoto vom Sticky Lab als PNG.

  python foto2png.py COM5 ziel.png         holt das Bild über USB (Befehl „f“)
  python foto2png.py f001.pbm ziel.png     wandelt ein Foto von der SD-Karte (/lab/fotos)

Braucht pyserial (nur für USB) und Pillow.
"""
import sys
from PIL import Image

W, H = 800, 480


def ueber_usb(port):
    import serial
    with serial.Serial(port, 115200, timeout=10) as s:
        s.reset_input_buffer()
        s.write(b"f")
        while True:
            zeile = s.readline()
            if not zeile:
                raise SystemExit("keine Antwort")
            if zeile.startswith(b"FOTO "):
                n = int(zeile.split()[1])
                break
        if n == 0:
            raise SystemExit("Sticky konnte kein Foto machen")
        s.readline(); s.readline()           # PBM-Kopf „P4“ und „800 480“
        daten = s.read(n)
        if len(daten) != n:
            raise SystemExit(f"nur {len(daten)} von {n} Byte")
        return daten


def main():
    quelle, ziel = sys.argv[1], sys.argv[2]
    if quelle.upper().startswith("COM") or quelle.startswith("/dev/"):
        bild = Image.frombytes("1", (W, H), ueber_usb(quelle), "raw", "1;I")
    else:
        bild = Image.open(quelle)
    bild.convert("L").save(ziel)
    print(ziel)


if __name__ == "__main__":
    main()
