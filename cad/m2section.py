"""A section through the right-hand post, for the figure that shows what holds the screen: out/mesh_section/."""
import json, pathlib
from build123d import *
import m2
from m2params import *

ROOT = pathlib.Path(__file__).resolve().parents[1]
out = ROOT / "out/mesh_section"
out.mkdir(parents=True, exist_ok=True)
cut = Pos(POST[2] - 50, 30, 10) * Box(100, 200, 200)      # keep x <= the post's axis
items = []
for name, shape, mat in [("front", m2.front(), "print_black"), ("back", m2.back(), "print_black"),
                         ("tray", m2.tray(), "white"), ("module", m2.module(), "pcb"),
                         ("battery", m2.battery(), "silver"), ("as7341", m2.as7341(), "pcb"),
                         ("encoder", m2.encoder(), "silver"), ("dial", m2.dial(), "print_black"),
                         ("dome", m2.dome(), "dome"), ("measure", m2.measure_key(), "red"),
                         ("power", m2.power_key(), "print_black")]:
    s = shape if name in ("module", "as7341", "encoder", "battery") else shape & cut   # the bought parts lie inside it
    f = out / f"{name}.stl"
    export_stl(s, str(f), tolerance=0.05 if name == "module" else 0.02, angular_tolerance=0.15)
    items.append({"name": name, "file": str(f), "mat": mat})
(out / "manifest.json").write_text(json.dumps(items, indent=1))
print(out)
