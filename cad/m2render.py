"""Meshes and a manifest for render/white.py: out/mesh_<tag>/. python m2render.py [tag] [--open] [--inside]"""
import json, pathlib, sys
from build123d import *
from PIL import Image
import m2
from m2params import *

ROOT = pathlib.Path(__file__).resolve().parents[1]


def slab(z0, z1):
    return Pos(0, 30, (z0 + z1) / 2) * Box(200, 200, z1 - z0)


def main(tag="m2", inside=False, dome_off=False, explode=0.0, open_=False):
    out = ROOT / f"out/mesh_{tag}"
    out.mkdir(parents=True, exist_ok=True)
    items = []
    e = explode

    def add(name, shape, mat, d=(0, 0, 0), tol=0.012):
        f = out / f"{name}.stl"
        export_stl(Pos(*d) * shape, str(f), tolerance=tol, angular_tolerance=0.1)
        items.append({"name": name, "file": str(f), "mat": mat})

    txt, dot = m2.face_marks()
    add("front", m2.front(), "print_black")
    add("name", txt & slab(0.06, 0.3), "ink_light")
    add("dot", dot & slab(0.06, 0.4), "red")
    if not open_:
        add("back", m2.back(), "print_black", (0, 0, 14 * e))
    dial = m2.dial()
    add("dial", dial, "print_black", (0, 10 * e, 0))
    add("dial_dot", m2.dial_dot(), "red", (0, 10 * e, 0))
    if dome_off:
        add("dome", Pos(0, H, 0) * m2.dome().mirror(Plane.XZ), "dome")   # parked in the seat under the foot
    else:
        add("dome", m2.dome(), "dome", (0, 14 * e, 0))
    add("measure", m2.measure_key(), "red", (-10 * e, 0, 0))
    add("power", m2.power_key(), "print_black", (-10 * e, 0, 0))
    # the module: its glass (black, glossy) and frame, from the maker's model
    mod = m2.module()
    if not (inside or e):   # only what shows: the glass and frame
        mod = sorted(mod.solids(), key=lambda so: -so.volume)[0]
    add("module", mod, "pcb", (0, 0, -10 * e), 0.05)
    if inside or e:
        add("tray", m2.tray(), "print_black", (0, 0, 6 * e))
        add("battery", m2.battery(), "silver", (0, 0, 10 * e))
        add("as7341", m2.as7341(), "pcb", (0, 6 * e, 0), 0.03)
        add("encoder", m2.encoder(), "silver", (0, 6 * e, 0), 0.03)
    png = out / "screen.png"
    ui = ROOT / "out/ui/screen368.png"
    if not ui.exists():
        u = Image.open(ROOT / "out/ui/01_aperture.ppm")
        c = Image.new("RGB", (368, 448), (0, 0, 0))
        s = 448 / 240 * 0.92
        u2 = u.resize((int(135 * s), int(240 * s)), Image.LANCZOS)
        c.paste(u2, ((368 - u2.width) // 2, (448 - u2.height) // 2))
        c.save(ui)
    Image.open(ui).save(png)
    x0, y0, x1, y1, z = m2.screen_quad()
    items.append({"name": "screen", "quad": [x0, y0, x1, y1, z - 10 * e], "image": str(png), "strength": 2.2})
    (out / "manifest.json").write_text(json.dumps(items, indent=1))
    print(out, len(items))


if __name__ == "__main__":
    a = sys.argv[1:]
    main(a[0] if a and not a[0].startswith("-") else "m2", "--inside" in a, "--dome-off" in a,
         4.0 if "--explode" in a else 0.0, "--open" in a)
