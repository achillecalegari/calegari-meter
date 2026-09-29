"""Calegari Meter: the printed parts, and the bought ones in place.

Printed (orientation on the bed in brackets):
  front     the shell: face, sides, top, bottom; the window, keys, USB-C, the port, the dial's bushing   (face down)
  back      the lid, snapped on behind; the shadow line where it meets the front                     (back down)
  tray      between the module's back and the battery                                                (flat)
  dial      knurled, on the encoder's shaft                                                          (top down)
  measure   the red key, snaps through the right side                                                (cap down)
  power     the power key                                                                            (cap down)
  dome      translucent white: pressed into the seat on top for incident light and colour            (dome up, 0.08 mm)
"""
import math, pathlib
from functools import lru_cache
from build123d import *
from m2params import *
from util import *

ROOT = pathlib.Path(__file__).resolve().parents[1]
FONT = str(ROOT / "tools/fonts/InterStatic-SemiBold.ttf")
XI = W / 2 - WALL                     # inside of the side walls
Y_IN0, Y_IN1 = WALL, H - WALL         # inside of the bottom and top walls
Z_IN0, Z_IN1 = FACE_T, T - WALL       # inside: behind the face, in front of the back
TOP_Y = H


def M(mx, my, mz):
    """Module coordinates -> meter."""
    return (-mx, my + MOD_Y, GLASS_Z - mz)


MOD_LOC = Pos(0, MOD_Y, GLASS_Z) * Rot(0, 180, 0)
AS_LOC = Pos(AS_X - 12.7, AS_Y0, T / 2 + 8.89) * Rot(-90, 0, 0)
CHIP = (AS_X - 12.7 + AS_CHIP[0], AS_Y0 + AS_CHIP[2], T / 2 + 8.89 - AS_CHIP[1])
AS_Z0, AS_Z1 = T / 2 + 8.89 - AS_BOARD[1], T / 2 + 8.89          # the board's front and back edges
ENC_X = -(XI - ENC_BODY[0] / 2 - 0.3)                            # the dial, over the photographer's right
ENC_Z = (Z_IN0 + Z_IN1) / 2
KEY_Z = GLASS_Z - KEY_MZ


def plate(w, h, r, z0, z1, x=0.0, y=0.0):
    return Pos(x, y, z0) * extrude(RectangleRounded(w, h, r), amount=z1 - z0)


def profile(inset=0.0, rt=None):
    """The block seen from the front: soft lower corners concentric with the glass, a tighter top plate."""
    w, h = W - 2 * inset, H - 2 * inset
    rb, rt = max(R_FACE - inset, 0.4), rt if rt is not None else max(R_TOP - inset, 0.4)
    x0, x1, y0, y1 = -w / 2, w / 2, H / 2 - h / 2, H / 2 + h / 2
    with BuildSketch() as sk:
        with BuildLine():
            Polyline((x0 + rb, y0), (x1 - rb, y0))
            RadiusArc((x1 - rb, y0), (x1, y0 + rb), -rb)
            Line((x1, y0 + rb), (x1, y1 - rt))
            RadiusArc((x1, y1 - rt), (x1 - rt, y1), -rt)
            Line((x1 - rt, y1), (x0 + rt, y1))
            RadiusArc((x0 + rt, y1), (x0, y1 - rt), -rt)
            Line((x0, y1 - rt), (x0, y0 + rb))
            RadiusArc((x0, y0 + rb), (x0 + rb, y0), -rb)
        make_face()
    return sk.sketch


def outline(z0, z1, inset=0.0, rt=None):
    return Pos(0, 0, z0) * extrude(profile(inset, rt), amount=z1 - z0)


def hollow(z0, z1):
    """Inside: square top corners, so the encoder and the AS7341 reach them (the walls there are thicker)."""
    return outline(z0, z1, WALL, rt=0.3)


def chamfer_edges(part, z, amount):
    """Chamfer the outline's edges in the plane z: the face, the back, the seam."""
    es = part.edges().filter_by(lambda e: abs(e.bounding_box().min.Z - z) < 1e-3 and abs(e.bounding_box().max.Z - z) < 1e-3)
    es = [e for e in es if max(abs(e.bounding_box().min.X), abs(e.bounding_box().max.X)) > W / 2 - R_FACE - 0.01
          or e.bounding_box().min.Y < R_FACE + 0.01 or e.bounding_box().max.Y > H - R_TOP - 0.01]
    return schamfer(part, es, amount)


def along_y(x, y, z, sk_or_solid):
    """A shape built along +Z, turned to point +Y at (x, y, z)."""
    return Pos(x, y, z) * Rot(-90, 0, 0) * sk_or_solid


def along_x(x, y, z, solid, sign=1):
    """A shape built along +Z, turned to point +X (sign 1) or -X (sign -1)."""
    return Pos(x, y, z) * Rot(0, 90 * sign, 0) * solid


# ------------------------------------------------------------------------------------------ marks

def spaced(word, size, track):
    from fontTools.ttLib import TTFont
    ft = TTFont(FONT)
    upm, cmap, hmtx = ft["head"].unitsPerEm, ft.getBestCmap(), ft["hmtx"]
    x, out = 0.0, []
    for ch in word:
        out.append(Pos(x, 0) * Text(ch, size, font_path=FONT, align=(Align.MIN, Align.MIN)))
        x += hmtx[cmap[ord(ch)]][0] * size / upm + track
    return Sketch() + out


def face_marks():
    """The name on the band's left, the red dot on its right; engraved 0.3 deep, read from the front
    (mirrored in X, since the face is seen from -Z)."""
    t = spaced("CALEGARI", 1.75, 0.5)
    bb = t.bounding_box()
    yb = BEZEL + GH + BAND / 2
    txt = extrude(Pos(-bb.min.X, -(bb.min.Y + bb.max.Y) / 2) * t, amount=0.31).mirror(Plane.YZ)
    tb = txt.bounding_box()
    txt = Pos(W / 2 - BEZEL - tb.max.X, yb, -0.01) * txt
    dot = Pos(-(W / 2 - BEZEL - 1.1), yb, -0.01) * Cylinder(1.1, 0.41, align=(Align.CENTER, Align.CENTER, Align.MIN))
    return txt, dot


# ------------------------------------------------------------------------------------------ front shell

def port_radius():
    """Reflected light: the port's rim at the top surface is the stop; its half-angle from the chip is CONE_HALF."""
    return (TOP_Y - DOME_SEAT[1] - CHIP[1]) * math.tan(math.radians(CONE_HALF))


def key_positions():
    return (MOD_Y + KEYS_MY[1], KEY_Z), (MOD_Y + KEYS_MY[0], KEY_Z)


def lid_snaps():
    """(x, y) of the lid's tongues, down the inside of the side walls near the seam."""
    return [(XI, 14.0), (XI, 40.0), (-XI, 10.0), (-XI, 40.0)]


def window():
    """The glass is flush with the face. The window meets its shoulder LIP_DEPTH down and follows the shoulder's
    curve to the full outline, so the frame covers the curve and only the flat glass shows; behind, the pocket
    for the module's frame."""
    gap = 0.08
    pts = [(-d, w, h) for d, w, h in GLASS_SHOULDER if -d >= LIP_DEPTH - 1e-6]
    secs = []
    w0, h0 = pts[0][1], pts[0][2]
    secs.append(Pos(0, MOD_Y, -0.02) * RectangleRounded(2 * (w0 + gap), 2 * (h0 + gap), GR - (GW / 2 - w0) + gap))
    for d, w, h in pts:
        z = GLASS_Z + d
        secs.append(Pos(0, MOD_Y, z) * RectangleRounded(2 * (w + gap), 2 * (h + gap), GR - (GW / 2 - w) + gap))
    cut = Solid()
    for a, b in zip(secs, secs[1:]):
        if abs(a.location.position.Z - b.location.position.Z) > 1e-4:
            cut = cut + loft([a, b])
    cut += plate(2 * (w0 + gap), 2 * (h0 + gap), GR - (GW / 2 - w0) + gap, -1, 0.0, 0, MOD_Y)
    depth_full = GLASS_Z + 1.0
    cut += plate(GW + 0.3, GH + 0.3, GR + 0.15, depth_full, FACE_T + 1, 0, MOD_Y)
    return cut


def front_ribs():
    out = []
    # the module: four guides from the side walls, touching its frame
    for s in (1, -1):
        for yy in (MOD_Y - GH / 2 + 5.5, MOD_Y + GH / 2 - 5.5):
            out.append(box_at(*sorted((s * (GW / 2 + 0.12), s * (XI + 0.3))), yy - 1.5, yy + 1.5, Z_IN0 - 0.2, GLASS_Z + MOD_BACK))
    # the AS7341: a ledge under it from each side wall; its front edge in a groove in the face wall (cut below)
    for x0, x1 in ((AS_X - AS_BOARD[0] / 2 - 1.0, AS_X - AS_BOARD[0] / 2 + 1.0), (AS_X + AS_BOARD[0] / 2 - 1.2, XI + 0.3)):
        out.append(box_at(x0, x1, AS_Y0 - 1.2, AS_Y0, Z_IN0 - 0.3, Z_IN0 + 3.0))
    out.append(encoder_cheek(-1))
    return out


def encoder_cheek(s):
    """A cheek that holds the encoder's body square under the top wall: s -1 toward the face, +1 toward the back."""
    zc = ENC_Z + s * (ENC_BODY[1] / 2 + 0.6)
    # it reaches down to the face wall (or up to the back): printed on it, never hanging in the air
    z0, z1 = (Z_IN0 - 0.1, zc + 0.6) if s < 0 else (zc - 0.6, Z_IN1 + 0.1)
    return box_at(ENC_X - ENC_BODY[0] / 2 + 1, ENC_X + ENC_BODY[0] / 2 - 1, Y_IN1 - ENC_BODY[2] + 1.5, Y_IN1 + 0.3, z0, z1)


@lru_cache(maxsize=None)
def front():
    f = outline(0, SEAM_Z)
    f = chamfer_edges(f, 0, CH)
    f = chamfer_edges(f, SEAM_Z, SEAM_CH)
    f -= hollow(Z_IN0, SEAM_Z + 1)
    f -= window()
    # the AS7341's front edge sits in a groove in the face wall
    f -= box_at(AS_X - AS_BOARD[0] / 2 - 0.15, AS_X + AS_BOARD[0] / 2 + 0.15, AS_Y0 - 0.1, AS_Y0 + 1.57 + 0.1,
                AS_Z0 - 0.15, Z_IN0 + 0.1)
    # the right side: the measuring key in its well, the power key, the USB-C
    (ym, zk), (yp, _) = key_positions()
    xs = -W / 2
    f -= along_x(xs, ym, zk, Cylinder(MEASURE[2] / 2, 2 * MEASURE_WELL))
    f -= along_x(xs, ym, zk, Cylinder(1.75, 8))
    pl, pw, _ = POWER
    f -= along_x(xs, yp, zk, extrude(SlotOverall(pl + 0.5, pw + 0.5).rotate(Axis.Z, 90), amount=1.2, both=True))
    f -= along_x(xs, yp, zk, extrude(SlotOverall(pl - 2.0, 1.8).rotate(Axis.Z, 90), amount=6, both=True))
    _, uy, uz = M(0, USB_MY, USB_MZ)
    f -= along_x(xs, uy, uz, extrude(SlotOverall(12.4, 6.6).rotate(Axis.Z, 90), amount=5.0, both=True))
    # the top: the dome's seat and the port, the stop for reflected light at the seat's floor
    cx, cy, cz = CHIP
    sd, sdep = DOME_SEAT
    f -= along_y(cx, TOP_Y - sdep, cz, Cylinder(sd / 2, 3, align=Z_UP))
    r = port_radius()
    f -= along_y(cx, cy + 0.3, cz, Cone(max(r - 0.9, 0.9), r, TOP_Y - sdep - cy - 0.3 + 0.01, align=Z_UP))
    # a window over the AS7341's LED, under the dome's flange: with the dome on, the flange carries the LED's
    # light round to the port; with it off, the light leaves. That is how the meter knows the dome is on
    f -= along_y(cx, TOP_Y - WALL - 0.3, cz - LED_WIN[1], Cylinder(LED_WIN[0] / 2, WALL - sdep + 0.31, align=Z_UP))
    # the encoder's bushing threads into the top wall; its locating tab in a slot
    f -= along_y(ENC_X, Y_IN1 - 1, ENC_Z, Cylinder(ENC_BUSH[0] / 2 - 0.15, 4, align=Z_UP))
    f -= box_at(ENC_X - 0.6, ENC_X + 0.6, Y_IN1 - 0.01, Y_IN1 + 0.9, ENC_Z + 5.5 - 0.7, ENC_Z + 5.5 + 0.7)
    # the dome's parking seat, under the foot
    f -= along_y(cx, -0.01, cz, Cylinder(sd / 2, sdep + 0.01, align=Z_UP))
    # grooves near the seam for the lid's snaps
    for x, y in lid_snaps():
        s = 1 if x > 0 else -1
        f -= box_at(*sorted((x - s * 0.1, x + s * 0.45)), y - 3.3, y + 3.3, SEAM_Z - 2.6, SEAM_Z - 1.8)
    skin = outline(-1, SEAM_Z + 1, 0.5)
    for rib in front_ribs():
        f += rib & skin
    txt, dot = face_marks()
    return f - txt - dot


# ------------------------------------------------------------------------------------------ back lid

@lru_cache(maxsize=None)
def back():
    b = outline(SEAM_Z, T)
    b = chamfer_edges(b, T, CH)
    b = chamfer_edges(b, SEAM_Z, SEAM_CH)
    b -= hollow(SEAM_Z - 1, Z_IN1)
    skin = outline(SEAM_Z - 6, T, 0.5)
    ribs = []
    # tongues down into the front shell, with a ridge each
    for x, y in lid_snaps():
        s = 1 if x > 0 else -1
        ribs.append(box_at(*sorted((x - s * 1.2, x - s * 0.08)), y - 3, y + 3, SEAM_Z - 4.0, SEAM_Z + 0.5))
        ribs.append(box_at(*sorted((x - s * 1.2, x + s * 0.4)), y - 3, y + 3, SEAM_Z + 0.02, SEAM_Z + 1.2))   # tied into the wall
        ribs.append(Pos(x - s * 0.08, y, SEAM_Z - 2.2) * Rot(90, 0, 0) * Rot(0, 0, 45) * Box(0.5, 0.5, 5.4))
    # the battery: two ribs over it, not touching (a cell must not be squeezed)
    for yy in (MOD_Y - 12, MOD_Y + 12):
        ribs.append(box_at(-BAT[1] / 2 + 3, BAT[1] / 2 - 3, yy - 0.8, yy + 0.8, BAT_Z0 + BAT[0] + BAT_GAP, Z_IN1 + 0.3))
    ribs.append(encoder_cheek(1) + box_at(ENC_X - 3, ENC_X + 3, Y_IN1 - 2, Y_IN1 + 0.3, ENC_Z + ENC_BODY[1] / 2 + 0.6, Z_IN1 + 0.3))
    for r in ribs:
        b += r & skin
    # four fingers round the cell's corners keep it in its place, with room all round
    bw, bh = BAT[1] / 2 + BAT_GAP, BAT[2] / 2 + BAT_GAP
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * bw, MOD_Y + sy * bh
            zf = BAT_Z0 + 2.0
            b += box_at(*sorted((cx, cx + sx * 0.8)), *sorted((cy + sy * 0.8, cy - sy * 6)), zf, Z_IN1 + 0.3)
            b += box_at(*sorted((cx + sx * 0.8, cx - sx * 6)), *sorted((cy, cy + sy * 0.8)), zf, Z_IN1 + 0.3)
    # the two posts that hold the module: down to the tray's springs, pressing them SPRING[3]
    d, py, px = POST
    for sx in (-1, 1):
        zt = BAT_Z0 - SPRING[3]
        b += Pos(sx * px, py, zt) * Cylinder(d / 2, Z_IN1 + 0.3 - zt, align=(Align.CENTER, Align.CENTER, Align.MIN))
    # the AS7341's back edge sits in a groove in the lid
    b -= box_at(AS_X - AS_BOARD[0] / 2 - 0.15, AS_X + AS_BOARD[0] / 2 + 0.15, AS_Y0 - 0.1, AS_Y0 + 1.57 + 0.1,
                Z_IN1 - 0.1, AS_Z1 + 0.15)
    return b


# ------------------------------------------------------------------------------------------ small parts

@lru_cache(maxsize=None)
def tray():
    """Between the module and the battery, wall to wall. Feet stand on the board where it has no parts; two
    leaf springs, cut in the plate and printed flat, take the lid's posts: the lid holds the module against the
    window with a light, even force. Printed plate down, feet up."""
    z0, z1 = BAT_Z0 - TRAY_T, BAT_Z0
    y0, y1 = Y_IN0 + 0.2, MOD_Y + GH / 2 + 2.0
    t = plate(2 * (XI - 0.2), y1 - y0, R_FACE - WALL - 0.2, z0, z1, 0, (y0 + y1) / 2)
    for x0, x1, fy0, fy1 in TRAY_FEET:
        t += box_at(x0, x1, fy0, fy1, PCB_BACK, z0 + 0.01)
    # the leaf springs: a tongue along y, free at its far end, the post on its tip
    L, w, slot, _ = SPRING
    d, py, px = POST
    for s in (-1, 1):
        x = s * px
        ya, yb = py - L + d / 2, py + d / 2 + 0.4          # the tongue: root at ya, tip past the post
        t -= box_at(x - w / 2 - slot, x + w / 2 + slot, ya, yb + slot, z0 - 1, z1 + 1)
        t += box_at(x - w / 2, x + w / 2, ya - 0.01, yb, z0, z1)
    # the battery's plug comes up through here from its socket on the board's edge; the header's wires there
    x0, py0, py1 = BAT_PLUG
    t -= box_at(x0, XI, py0, py1, z0 - 5, z1 + 1)
    t -= box_at(-10.5, 10.5, 41.0, y1 + 1, z0 - 5, z1 + 1)
    return t


@lru_cache(maxsize=None)
def dial():
    d, h = DIAL
    y0 = TOP_Y + 0.4
    dl = along_y(ENC_X, y0, ENC_Z, Cylinder(d / 2, h, align=Z_UP))
    dl = schamfer(dl, dl.edges().filter_by(GeomType.CIRCLE), 0.5)
    gw, gd, n = DIAL_KNURL
    cuts = []
    for i in range(n):
        a = 2 * math.pi * i / n
        cuts.append(Pos(ENC_X + d / 2 * math.cos(a), y0 + h / 2, ENC_Z + d / 2 * math.sin(a)) *
                    Rot(0, -math.degrees(a), 0) * Box(2 * gd, h - 1.6, gw))
    dl -= Compound(cuts)
    # inside: room for the bushing, the D bore for the shaft
    bush_top = Y_IN1 + ENC_BUSH[1]
    dl -= along_y(ENC_X, y0 - 0.01, ENC_Z, Cylinder(ENC_BUSH[0] / 2 + 0.6, bush_top - y0 + 0.4, align=Z_UP))
    dl -= d_shaft(y0 - 1, Y_IN1 + ENC_SHAFT[1] + 0.3, grow=0.05)
    dl -= dial_dot()                              # sunk 0.35 into the top, filled with red
    return dl


def d_shaft(y0, y1, grow=0.0):
    sd, _, flat = ENC_SHAFT
    s = along_y(ENC_X, y0, ENC_Z, Cylinder(sd / 2 + grow, y1 - y0, align=Z_UP))
    return s - box_at(ENC_X - sd, ENC_X + sd, y0 - 1, y1 + 1, ENC_Z + sd / 2 - (sd - flat) + grow, ENC_Z + sd)


def dial_dot():
    d, h = DIAL
    return along_y(ENC_X, TOP_Y + 0.4 + h - 0.35, ENC_Z - (d / 2 - 1.6), Cylinder(0.7, 0.4, align=Z_UP))


@lru_cache(maxsize=None)
def dome():
    """Translucent white: a half sphere on a flange that presses into the round seat on top, three crush ribs
    round it. Off, it parks in the same seat under the foot."""
    cx, _, cz = CHIP
    sd, sdep = DOME_SEAT
    y0 = TOP_Y - sdep
    fl = along_y(cx, y0, cz, Cylinder(sd / 2 - 0.08, sdep + 0.6, align=Z_UP))
    for a in (0, 120, 240):
        ra = sd / 2 - 0.08
        fl += along_y(cx + ra * math.cos(math.radians(a)), y0, cz + ra * math.sin(math.radians(a)), Cylinder(0.22, sdep, align=Z_UP))
    yc = TOP_Y + 0.6
    d = Pos(cx, yc, cz) * Sphere(DOME_R)          # solid: no roof to bridge, and a deeper diffuser
    d -= box_at(cx - 10, cx + 10, yc - 10, yc - 0.1, cz - 10, cz + 10)   # into the flange: one clean body
    return fl + d


def measure_key():
    """The red key: a flat top with a fine chamfer, proud of its well; a split stem snaps through the wall."""
    (ym, zk), _ = key_positions()
    d, proud, _ = MEASURE
    xo = -W / 2 + MEASURE_WELL                  # the well's floor
    k = along_x(xo, ym, zk, Cylinder(d / 2, MEASURE_WELL + proud, align=Z_UP), -1)
    k = schamfer(k, k.edges().filter_by(GeomType.CIRCLE).group_by(Axis.X)[0], 0.35)
    tip = -KEY_OUT - KEY_TRAVEL
    k += along_x(xo, ym, zk, Cylinder(1.5, tip - xo, align=Z_UP))
    k += barb(xo + WALL - MEASURE_WELL, ym, zk, 1.5)
    return k


def power_key():
    _, (yp, zk) = key_positions()
    pl, pw, proud = POWER
    xo = -W / 2 + 0.6
    k = along_x(xo, yp, zk, extrude(SlotOverall(pl, pw).rotate(Axis.Z, 90), amount=0.6 + proud), -1)
    k = schamfer(k, k.edges().group_by(Axis.X)[0], 0.3)
    tip = -KEY_OUT - KEY_TRAVEL
    k += along_x(xo, yp, zk, extrude(SlotOverall(pl - 2.4, 1.4).rotate(Axis.Z, 90), amount=tip - xo))
    k += barb(xo + WALL - 0.6, yp, zk, 0.7, span=2.2)
    return k


def barb(x_in, y, z, r, span=None):
    """Two prongs that spring out past the inside of the wall."""
    b = along_x(x_in + 0.1, y, z, Cone(r + 0.3, r - 0.1, 0.8, align=Z_UP))
    b -= box_at(x_in, x_in + 3, y - 0.3, y + 0.3, z - 3, z + 3)
    return b


# ------------------------------------------------------------------------------------------ bought parts

@lru_cache(maxsize=None)
def vendor(name):
    return import_step(str(ROOT / f"vendor/{name}"))


def module():
    return MOD_LOC * vendor("ws_amoled18/ESP32-S3-Touch-AMOLED-1.8-3D.stp")


def as7341():
    return AS_LOC * vendor("as7341.step")


def encoder():
    """EC11: body, bushing, D shaft, locating tab; pins bent down."""
    bx, bz, bh = ENC_BODY
    body_ = box_at(ENC_X - bx / 2, ENC_X + bx / 2, Y_IN1 - bh, Y_IN1, ENC_Z - bz / 2 + 0.7, ENC_Z + bz / 2 - 0.7)
    bush = along_y(ENC_X, Y_IN1, ENC_Z, Cylinder(ENC_BUSH[0] / 2, ENC_BUSH[1], align=Z_UP))
    shaft = d_shaft(Y_IN1, Y_IN1 + ENC_SHAFT[1])
    tab = box_at(ENC_X - 0.5, ENC_X + 0.5, Y_IN1, Y_IN1 + 0.8, ENC_Z + 5.5 - 0.6, ENC_Z + 5.5 + 0.6)
    pins = box_at(ENC_X - 3.5, ENC_X + 3.5, Y_IN1 - bh - 2.0, Y_IN1 - bh, ENC_Z - 6.0, ENC_Z - 5.0)
    return body_ + bush + shaft + tab + pins


def battery():
    th, wd, ht = BAT
    b = box_at(-wd / 2, wd / 2, MOD_Y - ht / 2, MOD_Y + ht / 2, BAT_Z0, BAT_Z0 + th)
    return schamfer(b, b.edges().filter_by(Axis.Y), 1.0)


def screen_quad():
    return (-AW / 2, MOD_Y - AH / 2, AW / 2, MOD_Y + AH / 2, GLASS_Z - 0.02)


if __name__ == "__main__":
    print(f"meter {W:.1f} x {H:.1f} x {T} mm (+ dial {DIAL[1] + 0.4}, dome {DOME_R + 0.6}); seam at Z {SEAM_Z}")
    print(f"AS7341 chip at {tuple(round(v, 2) for v in CHIP)}, board Z {AS_Z0:.2f}..{AS_Z1:.2f}; port diameter {2 * port_radius():.2f}")
    print(f"encoder at X {ENC_X:.2f} Z {ENC_Z:.2f}; keys {key_positions()}")
