"""Export everything needed to print the Calegari Meter.

print/step/     STEP of every printed part, in assembly coordinates
print/stl/      STL of every printed part, oriented on the bed as it should be printed
print/plates/   3MF plates for a 180 x 180 bed, one material and layer height per plate

python m2export.py
"""
import math, pathlib
import numpy as np
import trimesh
from build123d import *
import m2
from m2params import *

ROOT = pathlib.Path(__file__).resolve().parent.parent / "print"
BED = 180.0
EYE = np.eye(4)
R = trimesh.transformations.rotation_matrix


def mesh(shape, tol=0.02):
    vs, fs = shape.tessellate(tol, 0.15)
    return trimesh.Trimesh(np.array([(v.X, v.Y, v.Z) for v in vs]), np.array(fs), process=True)


def on_bed(m, T):
    m = m.copy()
    m.apply_transform(T)
    m.apply_translation(-m.bounds[0])
    return m


def catalogue():
    """(name, shape, orientation, copies, plate, why)"""
    return [
        ("front", m2.front(), EYE, 1, "black", "face down: the window, the name and the dot on the bed; the seam's edge on top"),
        ("back", m2.back(), R(math.pi, [1, 0, 0]), 1, "black", "back down: its chamfered edge on the bed, snaps and ribs on top"),
        ("tray", m2.tray(), EYE, 1, "black", "flat"),
        ("dial", m2.dial(), R(-math.pi / 2, [1, 0, 0]), 1, "black", "top down: the knurl's grooves stand vertical, the dot's pit on the bed"),
        ("power_key", m2.power_key(), R(-math.pi / 2, [0, 1, 0]), 1, "black", "cap down, stem up"),
        ("measure_key", m2.measure_key(), R(-math.pi / 2, [0, 1, 0]), 2, "red", "cap down, stem up; a spare"),
        ("dome", m2.dome(), R(math.pi / 2, [1, 0, 0]), 2, "dome", "flange down, 0.08 mm layers: the half sphere steps less; a spare"),
    ]


def pack(items, margin=10.0, gap=8.0):
    items = sorted(items, key=lambda it: -max(it[1].extents[:2]))
    parts, x, y, row = [], margin, margin, 0.0
    for label, m in items:
        w, d = m.extents[0], m.extents[1]
        if x + w > BED - margin:
            x, y, row = margin, y + row + gap, 0.0
        mm = m.copy()
        mm.apply_translation([x, y, 0])
        parts.append((label, mm))
        x, row = x + w + gap, max(row, d)
    lo = np.min([m.bounds[0][:2] for _, m in parts], axis=0)
    hi = np.max([m.bounds[1][:2] for _, m in parts], axis=0)
    sh = (BED - (hi - lo)) / 2 - lo
    for _, m in parts:
        m.apply_translation([sh[0], sh[1], 0])
    return parts


def write_plate(path, parts):
    scene = trimesh.Scene()
    for label, m in parts:
        scene.add_geometry(m, node_name=label, geom_name=label)
    scene.export(str(path))


PLATES = {
    "black": ("plate_1_black.3mf", "black PETG or ASA, 0.16 mm layers, 3 walls, 20 % infill, no supports"),
    "red": ("plate_2_red.3mf", "red PETG or ASA, 0.12 mm layers, 100 % infill, no supports"),
    "dome": ("plate_3_dome.3mf", "translucent white PETG (natural), 0.08 mm layers, 100 % infill, no supports"),
}

if __name__ == "__main__":
    for d in ("step", "stl", "plates"):
        (ROOT / d).mkdir(parents=True, exist_ok=True)
        for f in (ROOT / d).glob("*"):
            f.unlink()
    groups, report, bad = {}, [], []
    for name, shape, T, copies, grp, why in catalogue():
        export_step(shape, str(ROOT / "step" / f"{name}.step"))
        m = on_bed(mesh(shape, 0.01 if grp != "black" else 0.02), T)
        m.merge_vertices()
        bodies = [b for b in m.split(only_watertight=False) if b.is_watertight and abs(b.volume) > 1e-3]
        if len(bodies) == 1:
            m = bodies[0]                          # drop the odd stray triangle the tessellator leaves at a tangency
        if not m.is_watertight or m.body_count != 1:
            bad.append(name)
        m.export(str(ROOT / "stl" / f"{name}.stl"))
        for k in range(copies):
            groups.setdefault(grp, []).append((name if copies == 1 else f"{name}_{k + 1}", m))
        report.append(f"{name:12s} x{copies}  {grp:6s} {m.extents[0]:5.1f} x {m.extents[1]:5.1f} x {m.extents[2]:5.1f} mm  "
                      f"{m.volume / 1000:5.2f} cm3  {why}")
    for grp, (fname, how) in PLATES.items():
        write_plate(ROOT / "plates" / fname, pack(groups[grp]))
        report.append(f"{fname}: {', '.join(l for l, _ in groups[grp])}  ({how})")
    print("\n".join(report))
    print("not watertight or not one body:", bad or "none")
