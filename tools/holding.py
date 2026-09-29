"""The figure that shows what holds the screen: docs/img/holding.svg, a section across the meter through the
lid's posts, to scale (the springs' travel drawn three times larger)."""
import pathlib, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "cad"))
from m2params import *

OUT = pathlib.Path(__file__).resolve().parents[1] / "docs/img/holding.svg"
INK, GREY, SOFT, RED, LINE = "#111112", "#86868b", "#f5f5f7", "#c8102e", "#d8d8dc"
S = 13.0                              # px per mm
X0, Z0 = 60 + W / 2 * S, 96           # svg origin of x = 0, and the top of the drawing
ZMAX = T
out = []
add = out.append


def P(x, z):
    return X0 + x * S, Z0 + (ZMAX - z) * S


def rect(x0, x1, z0, z1, fill, stroke="none", sw=0, extra=""):
    (a, b), (c, d) = P(min(x0, x1), max(z0, z1)), P(max(x0, x1), min(z0, z1))
    add(f'<rect x="{a:.1f}" y="{b:.1f}" width="{c - a:.1f}" height="{d - b:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" {extra}/>')


def poly(pts, fill, stroke="none", sw=0):
    add(f'<polygon points="{" ".join(f"{P(x, z)[0]:.1f},{P(x, z)[1]:.1f}" for x, z in pts)}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')


Wd, Hd = 980, 470
add(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {Wd} {Hd}" width="{Wd}" height="{Hd}" font-family="Inter, Helvetica, Arial, sans-serif">')
add(f'<rect width="{Wd}" height="{Hd}" fill="#fff"/>')
add(f'<text x="40" y="36" font-size="22" font-weight="700" fill="{INK}">What holds the screen</text>')
add(f'<text x="40" y="60" font-size="14" fill="{GREY}">A section across the meter through the two posts, to scale; the springs\' travel drawn three times larger.</text>')

XI_ = W / 2 - WALL
k = 3.0                                # exaggeration of the springs' travel
d, py, px = POST
L, sw_, slot, press = SPRING
z0t, z1t = BAT_Z0 - TRAY_T, BAT_Z0
# the case: front shell to the seam, lid behind
body = "#2a2a2c"
rect(-W / 2, W / 2, 0, SEAM_Z, body)                    # front shell, then hollowed
rect(-W / 2, W / 2, SEAM_Z, T, "#3a3a3d")               # lid
rect(-XI_, XI_, FACE_T, T - WALL, "#fff")               # inside
rect(-16.45, 16.45, -0.1, 0.41, "#fff")                # the window, at the face
rect(-16.95, 16.95, 0.4, FACE_T + 0.01, "#fff")         # the pocket behind it, round the glass's full outline
add(f'<line x1="{P(-W / 2, SEAM_Z)[0]}" y1="{P(0, SEAM_Z)[1]}" x2="{P(-XI_, SEAM_Z)[0]}" y2="{P(0, SEAM_Z)[1]}" stroke="#fff" stroke-width="1.2"/>')
add(f'<line x1="{P(XI_, SEAM_Z)[0]}" y1="{P(0, SEAM_Z)[1]}" x2="{P(W / 2, SEAM_Z)[0]}" y2="{P(0, SEAM_Z)[1]}" stroke="#fff" stroke-width="1.2"/>')
# the module: the glass rolls from its flat top to its full outline over its shoulder; the window meets the
# shoulder at the face, and that is where it stops
sh = [(z, hx) for z, hx, _ in GLASS_SHOULDER]
pts = ([(-16.8, GLASS_Z + 6.67)] + [(-hx, GLASS_Z - z) for z, hx in sh[::-1]] + [(hx, GLASS_Z - z) for z, hx in sh]
       + [(16.8, GLASS_Z + 6.67)])
poly(pts, "#0c0c0e")
rect(-16.4, 16.6, PCB_BACK - 1.2, PCB_BACK, "#23405e")
for x0, x1, zt in ((-12, -6.5, 8.0), (-4, 3.5, 7.9), (5, 12.5, 8.3), (-15.5, -13.5, 7.6)):
    rect(x0, x1, PCB_BACK, zt, "#5d6f82")
# the tray, its foot on the board's edge, the springs pressed by the posts
tray = "#e9e7e2"
if px + sw_ / 2 + slot < XI_ - 0.2:
    rect(-(XI_ - 0.2), -(px + sw_ / 2 + slot), z0t, z1t, tray, INK, 0.8)
rect(-(px - sw_ / 2 - slot), px - sw_ / 2 - slot, z0t, z1t, tray, INK, 0.8)
if px + sw_ / 2 + slot < XI_ - 0.2:
    rect(px + sw_ / 2 + slot, XI_ - 0.2, z0t, z1t, tray, INK, 0.8)
rect(15.0, 15.9, PCB_BACK, z0t, tray, INK, 0.8)
for s in (-1, 1):
    x = s * px
    rect(x - sw_ / 2, x + sw_ / 2, z0t - press * k, z1t - press * k, RED, INK, 0.8)     # the spring's tip, pressed
    rect(x - d / 2, x + d / 2, z1t - press * k, T - WALL, "#3a3a3d")                    # the post
# the battery, and the room over it
rect(-BAT[1] / 2, BAT[1] / 2, BAT_Z0, BAT_Z0 + BAT[0], "#c9c9cc", INK, 0.6)
for s in (-1, 1):
    rect(s * 3, s * 12.5, BAT_Z0 + BAT[0] + BAT_GAP, T - WALL, "#3a3a3d")

# labels
def label(x, z, n, text, sub, lx, ly):
    ax, az = P(x, z)
    add(f'<line x1="{ax:.1f}" y1="{az:.1f}" x2="{lx - 8}" y2="{ly - 5}" stroke="{GREY}" stroke-width="1"/>')
    add(f'<circle cx="{ax:.1f}" cy="{az:.1f}" r="2.6" fill="{RED}"/>')
    add(f'<text x="{lx}" y="{ly}" font-size="14" font-weight="600" fill="{INK}">{n}  {text}</text>')
    add(f'<text x="{lx + 18}" y="{ly + 18}" font-size="12.5" fill="{GREY}">{sub}</text>')

LX = 700
label(16.45, 0.0, "1", "The window's rim", "the glass stops on it", LX, 440)
label(13.5, PCB_BACK - 0.6, "2", "The board", "pushed toward the face", LX, 384)
label(15.45, 8.6, "3", "A foot of the tray", "on the board, clear of its parts", LX, 328)
label(12, z1t - 0.6, "4", "The tray", "wall to wall", LX, 272)
label(px, z0t - press * k + 0.6, "5", "A leaf spring", f"pressed {press} mm, printed flat", LX, 216)
label(px, 16, "6", "A post in the lid", "the lid snaps on and presses", LX, 160)
label(13.5, BAT_Z0 + BAT[0] - 1.0, "7", "The battery", f"never squeezed: {BAT_GAP} mm to spare", LX, 104)
add(f'<text x="{P(0, 0)[0]:.0f}" y="{P(0, -0.6)[1] + 26:.0f}" font-size="12" text-anchor="middle" fill="{GREY}">the face, the glass 0.6 mm proud</text>')
add("</svg>")
OUT.write_text("\n".join(out))
print(OUT)
