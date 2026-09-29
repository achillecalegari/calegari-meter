"""Every pair of parts, printed and bought: how much they overlap (mm^3). Designed contacts have a budget."""
import itertools, sys, time
from build123d import *
import m2

BOUGHT = ("module", "as7341", "encoder", "battery")
BUDGET = {("front", "module"): 2.0,         # the window's edge holds the glass's shoulder; guides
          ("front", "as7341"): 1.0, ("back", "as7341"): 1.0,   # its edges in the grooves, on the ledges
          ("front", "encoder"): 6.0,        # the bushing threads into the top wall (0.15 mm of thread)
          ("back", "battery"): 9.0,         # two ribs press it 0.1 mm
          ("dial", "encoder"): 1.5,         # the D bore grips the shaft
          ("front", "dome"): 0.3,           # crush ribs in the seat
          ("front", "measure"): 0.6, ("front", "power"): 0.4,  # the barbs
          ("front", "back"): 1.0}           # the snaps' ridges


def solids(s):
    return list(s.solids()) if hasattr(s, "solids") else [s]


def overlap(a, b):
    tot = 0.0
    ba = a.bounding_box()
    for sb in solids(b):
        bb = sb.bounding_box()
        if (bb.min.X > ba.max.X or bb.max.X < ba.min.X or bb.min.Y > ba.max.Y or bb.max.Y < ba.min.Y
                or bb.min.Z > ba.max.Z or bb.max.Z < ba.min.Z):
            continue
        for sa in solids(a):
            try:
                tot += (sa & sb).volume
            except Exception:
                pass
    return tot


def parts():
    return {"front": m2.front(), "back": m2.back(), "tray": m2.tray(), "dial": m2.dial(), "dome": m2.dome(),
            "measure": m2.measure_key(), "power": m2.power_key(),
            "module": m2.module(), "as7341": m2.as7341(), "encoder": m2.encoder(), "battery": m2.battery()}


def main():
    t = time.time()
    p = parts()
    print(f"built in {time.time() - t:.0f} s")
    bad = 0
    for a, b in itertools.combinations(p, 2):
        if a in BOUGHT and b in BOUGHT:
            if {a, b} not in ({"module", "battery"}, {"module", "as7341"}, {"module", "encoder"}, {"as7341", "encoder"}):
                continue
        v = overlap(p[a], p[b])
        lim = BUDGET.get((a, b), BUDGET.get((b, a), 0.01))
        ok = v <= lim
        bad += not ok
        if v > 0.001 or not ok:
            print(f"{a:10s} {b:10s} {v:8.3f} mm3  budget {lim}  {'ok' if ok else 'PROBLEM'}")
    for n in ("front", "back", "tray", "dial", "dome", "measure", "power"):
        print(f"{n}: valid {p[n].is_valid}, {p[n].volume:.0f} mm3, {len(solids(p[n]))} solid(s)")
    print("problems:", bad)
    return bad


if __name__ == "__main__":
    sys.exit(main())
