"""The wiring figure: docs/img/wiring.svg. Seven wires, fourteen joints."""
import pathlib

OUT = pathlib.Path(__file__).resolve().parents[1] / "docs/img/wiring.svg"
HEADER = ["VBUS", "GND", "3V3", "GND", "TXD", "RXD", "IO14", "IO15", "IO17", "IO18", "IO38", "IO39", "IO40", "IO41", "IO42"]
INK, GREY, SOFT = "#111112", "#86868b", "#f5f5f7"
C = {"3V3": "#c8102e", "GND": "#111112", "SCL": "#d99a00", "SDA": "#2f6fd0", "A": "#2e9d57", "B": "#7a4fc9"}

W, H = 800, 640
P0, PITCH, YH = 88, 44, 150            # header: first pad x, pitch, y
YB = 360                               # the sensor's pads
YE = YB + 128                          # the encoder's, lower: side by side the two would touch
s = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
     f'font-family="Inter, Helvetica, Arial, sans-serif">', f'<rect width="{W}" height="{H}" fill="#fff"/>']
add = s.append


def px(i):
    return P0 + i * PITCH


def pad(x, y, on=True):
    add(f'<circle cx="{x}" cy="{y}" r="7" fill="#fff" stroke="{INK if on else "#b8b8bc"}" stroke-width="{1.8 if on else 1.2}"/>'
        f'<circle cx="{x}" cy="{y}" r="2.6" fill="{INK if on else "#b8b8bc"}"/>')


def path(pts, colour, hops=()):
    """An orthogonal wire; hops: x positions on a horizontal run where it jumps over another wire."""
    d = f"M {pts[0][0]} {pts[0][1]}"
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        if y0 == y1:
            for hx in sorted(hops, reverse=x1 < x0):
                if min(x0, x1) < hx < max(x0, x1):
                    sgn = 1 if x1 > x0 else -1
                    d += f" L {hx - 8 * sgn} {y0} A 8 8 0 0 {1 if sgn > 0 else 0} {hx + 8 * sgn} {y0}"
        d += f" L {x1} {y1}"
    add(f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="3.4" stroke-linecap="round" stroke-linejoin="round"/>')


add(f'<text x="40" y="46" font-size="22" font-weight="700" fill="{INK}">Seven wires, fourteen joints</text>')
add(f'<text x="40" y="72" font-size="14" fill="{GREY}">30 AWG silicone wire, about 60 mm each. The holes in the header are 0.7 mm at a 1.27 mm pitch.</text>')

# the Waveshare's header: its back, along the top edge
add(f'<rect x="{P0 - 30}" y="{YH - 44}" width="{PITCH * 14 + 60}" height="78" rx="14" fill="{SOFT}"/>')
add(f'<text x="{P0 - 18}" y="{YH - 24}" font-size="13" font-weight="600" fill="{INK}">Waveshare ESP32-S3-Touch-AMOLED-1.8</text>'
    f'<text x="{P0 + PITCH * 14 + 18}" y="{YH - 24}" font-size="12" text-anchor="end" fill="{GREY}">header on the back, top edge</text>')
used = {1, 2, 3, 6, 7, 8, 9}
for i, name in enumerate(HEADER):
    pad(px(i), YH, i in used)
    add(f'<text x="{px(i)}" y="{YH + 26}" text-anchor="middle" font-size="11" font-weight="{600 if i in used else 400}" '
        f'fill="{INK if i in used else GREY}">{name}</text>')

# the AS7341: its pads straight under the header's
as_pads = {"VIN": px(2), "GND": px(1), "SCL": px(6), "SDA": px(7)}
x0, x1 = px(1) - 34, px(7) + 34
add(f'<rect x="{x0}" y="{YB - 26}" width="{x1 - x0}" height="100" rx="14" fill="{SOFT}"/>')
add(f'<text x="{(x0 + x1) / 2}" y="{YB + 44}" text-anchor="middle" font-size="14" font-weight="600" fill="{INK}">Adafruit AS7341</text>'
    f'<text x="{(x0 + x1) / 2}" y="{YB + 62}" text-anchor="middle" font-size="12" fill="{GREY}">the light sensor, under the dome</text>')
for n, x in as_pads.items():
    pad(x, YB)
    add(f'<text x="{x}" y="{YB + 24}" text-anchor="middle" font-size="11" font-weight="600" fill="{INK}">{n}</text>')

# the encoder, to the right and lower
en_pads = {"A": px(8), "B": px(9), "C": px(11)}
x0, x1 = px(8) - 34, px(11) + 34
add(f'<rect x="{x0}" y="{YE - 26}" width="{x1 - x0}" height="100" rx="14" fill="{SOFT}"/>')
add(f'<text x="{(x0 + x1) / 2}" y="{YE + 44}" text-anchor="middle" font-size="14" font-weight="600" fill="{INK}">EC11 encoder</text>'
    f'<text x="{(x0 + x1) / 2}" y="{YE + 62}" text-anchor="middle" font-size="12" fill="{GREY}">the dial; C is the middle pin</text>')
for n, x in en_pads.items():
    pad(x, YE)
    add(f'<text x="{x}" y="{YE + 24}" text-anchor="middle" font-size="11" font-weight="600" fill="{INK}">{n}</text>')

y0, y1 = YH + 36, YB - 10
path([(px(1), y0), (px(1), y1)], C["GND"])
path([(px(2), y0), (px(2), y1)], C["3V3"])
path([(px(6), y0), (px(6), y1)], C["SCL"])
path([(px(7), y0), (px(7), y1)], C["SDA"])
path([(px(8), y0), (px(8), YE - 10)], C["A"])
path([(px(9), y0), (px(9), YE - 10)], C["B"])
yr = YH + 90
path([(px(3), y0), (px(3), yr), (px(11), yr), (px(11), YE - 10)], C["GND"], hops=(px(6), px(7), px(8), px(9)))

legend = [("3V3", "3V3 to VIN"), ("GND", "GND to GND, twice"), ("SCL", "GPIO14 to SCL"),
          ("SDA", "GPIO15 to SDA"), ("A", "GPIO17 to A"), ("B", "GPIO18 to B")]
for k, (c, t) in enumerate(legend):
    x, y = 58 + (k % 3) * 240, 598 + (k // 3) * 24
    add(f'<rect x="{x}" y="{y - 7}" width="20" height="4" rx="2" fill="{C[c]}"/>'
        f'<text x="{x + 30}" y="{y - 1}" font-size="13" fill="{INK}">{t}</text>')
add("</svg>")
OUT.write_text("\n".join(s))
print(OUT)
