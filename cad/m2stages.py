"""Meshes for the assembly figures: out/mesh_stage<N>/, each stage adding the parts that go in at that step."""
import json, pathlib, sys
from build123d import *
import m2
from m2params import *

ROOT = pathlib.Path(__file__).resolve().parents[1]


def slab(z0, z1):
    return Pos(0, 30, (z0 + z1) / 2) * Box(200, 200, z1 - z0)


def stage(n):
    out = ROOT / f"out/mesh_stage{n}"
    out.mkdir(parents=True, exist_ok=True)
    items = []

    def add(name, shape, mat, tol=0.012):
        f = out / f"{name}.stl"
        export_stl(shape, str(f), tolerance=tol, angular_tolerance=0.1)
        items.append({"name": name, "file": str(f), "mat": mat})

    txt, dot = m2.face_marks()
    add("front", m2.front(), "print_black")
    add("name", txt & slab(0.06, 0.3), "ink_light")
    add("dot", dot & slab(0.06, 0.4), "red")
    add("measure", m2.measure_key(), "red")
    add("power", m2.power_key(), "print_black")
    if n >= 2:
        add("module", m2.module(), "pcb", 0.05)
    if n >= 3:
        add("as7341", m2.as7341(), "pcb", 0.03)
        add("encoder", m2.encoder(), "silver", 0.03)
        add("dial", m2.dial(), "print_black")
        add("dial_dot", m2.dial_dot(), "red")
    if n >= 4:
        add("tray", m2.tray(), "print_black")
        add("battery", m2.battery(), "silver")
    if n >= 5:
        add("back", m2.back(), "print_black")
        add("dome", m2.dome(), "dome")
    (out / "manifest.json").write_text(json.dumps(items, indent=1))
    print(out, len(items))


if __name__ == "__main__":
    for n in map(int, sys.argv[1:] or ["1", "2", "3", "4"]):
        stage(n)
