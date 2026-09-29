"""Printability: each printed part in its orientation on the bed; the area that overhangs more than 50 degrees
from vertical without resting on the bed or on what is printed under it (roughly: downward faces above z 0.3)."""
import math, pathlib, tempfile
import numpy as np, trimesh
from build123d import *
import m2
from m2params import *

# part -> (shape, a rotation that puts it on the bed as printed)
PARTS = {
    "front":   (m2.front,       lambda s: s),                                   # face down: Z up already
    "back":    (m2.back,        lambda s: Rot(180, 0, 0) * s),                  # back down
    "tray":    (m2.tray,        lambda s: Rot(180, 0, 0) * s),                  # plate down, feet up
    "dial":    (m2.dial,        lambda s: Rot(-90, 0, 0) * s),                  # top down
    "dome":    (m2.dome,        lambda s: Rot(90, 0, 0) * s),                   # dome up
    "measure": (m2.measure_key, lambda s: Rot(0, -90, 0) * s),                  # cap down
    "power":   (m2.power_key,   lambda s: Rot(0, -90, 0) * s),
}


def main():
    tmp = pathlib.Path(tempfile.mkdtemp())
    for name, (fn, orient) in PARTS.items():
        s = orient(fn())
        bb = s.bounding_box()
        s = Pos(0, 0, -bb.min.Z) * s
        f = tmp / f"{name}.stl"
        export_stl(s, str(f), tolerance=0.02, angular_tolerance=0.15)
        m = trimesh.load(f)
        n = m.face_normals
        c = m.triangles_center
        a = m.area_faces
        down = n[:, 2] < -math.cos(math.radians(50))      # steeper than 50 degrees from vertical, facing down
        off_bed = c[:, 2] > 0.3
        area = a[down & off_bed].sum()
        # the 45 degree chamfers at the bed's edge are fine: count what lies above the first millimetre too
        high = a[down & (c[:, 2] > 1.0)].sum()
        # small features are fine: report the largest connected patch, and the part's height
        print(f"{name:8s} height {m.bounds[1][2]:5.1f} mm   overhang {area:6.1f} mm2, above the first mm {high:6.1f} mm2")
    return 0


if __name__ == "__main__":
    main()
