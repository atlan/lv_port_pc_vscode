"""Aufruf:  ~/Projects/bes/.venv/bin/python bes_gitter.py <quelle.png> <ziel.png> [Maschenweite 26] [Netzlinie 2.0] [Zielhöhe in px]
(braucht numpy, scipy, Pillow, contourpy -- die liegen in der bes-.venv)

Bes als 3D-Gitter: Silhouette und Binnenlinien der Zeichnung werden zu einem Relief
aufgeblasen, darueber liegt ein Netz aus Linien gleicher Bogenlaenge (wie Laengen- und
Breitenkreise), leicht von oben gesehen."""
import sys, numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage as ndi
import contourpy

QUELLE, ZIEL = sys.argv[1], sys.argv[2]
ABSTAND  = float(sys.argv[3]) if len(sys.argv) > 3 else 26.0   # Maschenweite in px Bogenlaenge
NETZ_B   = float(sys.argv[4]) if len(sys.argv) > 4 else 2.0    # Breite der Netzlinien in Quell-px
ZIEL_H   = int(sys.argv[5]) if len(sys.argv) > 5 else 0        # 0 = Quellgroesse behalten
NEIGUNG  = 0.45      # Blick von oben: Verschiebung je px Hoehe
SS       = 3         # Ueberabtastung beim Zeichnen
FARBE_NETZ   = (255, 153, 0)      # LCARS-Bernstein
FARBE_KONTUR = (255, 204, 102)    # heller fuer die Zeichnungslinien

img = Image.open(QUELLE).convert("RGBA")
a = np.asarray(img).astype(np.float32)
W, H = img.size
innen = a[..., 3] > 128
lum = 0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]
strich = innen & (lum < 45)                       # die schwarzen Linien der Zeichnung

# Relief: die ganze Figur woelbt sich, jede von Linien umschlossene Flaeche noch einmal fuer sich
d_figur  = ndi.distance_transform_edt(innen)
d_flaeche = ndi.distance_transform_edt(innen & ~strich)
h = 3.4 * np.sqrt(d_figur) + 1.8 * np.sqrt(d_flaeche)
h = ndi.gaussian_filter(h, 7.0) * innen

# Bogenlaenge entlang x (von der Mittelachse aus) und entlang y (von oben)
hy, hx = np.gradient(h)
dsx = np.sqrt(1.0 + hx ** 2); dsy = np.sqrt(1.0 + hy ** 2)
mitte = W // 2
Sx = np.cumsum(dsx, axis=1); Sx = Sx - Sx[:, mitte:mitte + 1]
Ty = np.cumsum(dsy, axis=0)
# geglaettet, sonst zittern die Netzlinien an jeder Falte des Reliefs
Sx = ndi.gaussian_filter(Sx, 7.0); Ty = ndi.gaussian_filter(Ty, 11.0)

def linien(feld):
    f = np.ma.array(feld, mask=~innen)
    gen = contourpy.contour_generator(z=f, corner_mask=True)
    lo, hi = float(f.min()), float(f.max())
    aus = []
    for wert in np.arange(np.floor(lo / ABSTAND) * ABSTAND, hi, ABSTAND):
        for pfad in gen.lines(wert):
            if len(pfad) >= 2: aus.append(pfad)
    return aus

def projiziere(p):
    x, y = p[:, 0], p[:, 1]
    hh = ndi.map_coordinates(h, [y, x], order=1, mode="nearest")
    return np.stack([x, y - NEIGUNG * hh], axis=1)

rand = int(NEIGUNG * h.max()) + 20
bild = Image.new("RGB", (W * SS, (H + rand) * SS), (0, 0, 0))
z = ImageDraw.Draw(bild)
n = 0
for feld in (Sx, Ty):
    for pfad in linien(feld):
        q = projiziere(pfad); q[:, 1] += rand
        z.line([(float(x) * SS, float(y) * SS) for x, y in q], fill=FARBE_NETZ, width=max(1, int(NETZ_B * SS)), joint="curve"); n += 1

# Zeichnungslinien und Umriss mit derselben Verschiebung daruebergelegt
yy, xx = np.mgrid[0:H + rand, 0:W].astype(np.float32)
quelle_y = yy - rand
for _ in range(4):                                  # y' = y - N*h(y)  nach y aufloesen
    hh = ndi.map_coordinates(h, [np.clip(quelle_y, 0, H - 1), xx], order=1, mode="nearest")
    quelle_y = (yy - rand) + NEIGUNG * hh
umriss = innen & ~ndi.binary_erosion(innen, iterations=3)
# schwarze FLAECHEN (Pupillen, Flecken) nur als Ring, duenne Striche bleiben ganz
strich_linie = strich & ~ndi.binary_erosion(strich, iterations=4)
kontur = (strich_linie | umriss).astype(np.float32)
k = ndi.map_coordinates(kontur, [np.clip(quelle_y, 0, H - 1), xx], order=1, mode="constant")
k[(quelle_y < 0) | (quelle_y > H - 1)] = 0
kb = Image.fromarray((np.clip(k, 0, 1) * 255).astype(np.uint8)).resize(bild.size, Image.LANCZOS)
bild.paste(Image.new("RGB", bild.size, FARBE_KONTUR), mask=kb)
ziel = (W, H + rand) if not ZIEL_H else (round(W * ZIEL_H / (H + rand)), ZIEL_H)
bild = bild.resize(ziel, Image.LANCZOS)
bild.save(ZIEL)
alpha = bild.convert("L").point(lambda v: min(255, int(v * 255 / 204)))
frei = Image.new("RGBA", bild.size, (255, 255, 255, 0)); frei.putalpha(alpha)
frei.save(ZIEL.replace(".png", "_transparent.png"))
print(f"{n} Netzlinien, Hoehe bis {h.max():.0f} px, Bild {bild.size}")
