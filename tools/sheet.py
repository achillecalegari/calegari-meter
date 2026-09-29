"""Contact sheet of the rendered screens: out/ui/*.ppm -> out/ui/sheet.png, each at 3x, pixels kept square."""
import pathlib
from PIL import Image
S, pad = 3, 24
files = sorted(pathlib.Path("out/ui").glob("*.ppm"))
w, h = Image.open(files[0]).size
cols = 4 if h > w else 2
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (cols * (w * S + pad) + pad, rows * (h * S + pad) + pad), (255, 255, 255))
for i, f in enumerate(files):
    im = Image.open(f).resize((w * S, h * S), Image.NEAREST)
    im.save(f.with_suffix(".png"))
    sheet.paste(im, (pad + (i % cols) * (w * S + pad), pad + (i // cols) * (h * S + pad)))
sheet.save("out/ui/sheet.png")
print(sheet.size)
