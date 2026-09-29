# Printing

Three plates in `print/plates/`, each one material at one layer height. They fit any bed from 180 x 180 mm. Every part is already turned the way it should lie; the single STLs in `print/stl/` are turned the same way, and `print/step/` has each part in its place in the assembly, for your own changes.

No part needs supports.

| Plate | Parts | Material | Layers | Walls, infill |
|---|---|---|---|---|
| `plate_1_black.3mf` | front shell, back lid, battery tray, dial, power key | black PETG or ASA, matte if you can | 0.16 mm | 3 walls, 20 % gyroid |
| `plate_2_red.3mf` | two red keys | red PETG or ASA | 0.12 mm | 100 % |
| `plate_3_dome.3mf` | two domes | translucent white PETG | 0.08 mm | 100 % |

## How each part lies, and why

| Part | On the bed | Why |
|---|---|---|
| front shell | face down | the face, with the window, the engraved name and the dot's pit, is the first layer: flat as the plate. Its edge is a 45 degree chamfer, not a round, so it prints clean. The seam's edge is on top |
| back lid | back down | the same: the back is the first layer, its edge a chamfer; the snaps and the ribs stand up from it |
| battery tray | flat | 0.8 mm |
| dial | top down | the knurl's grooves stand vertical, so each is a clean line in every layer; the red dot's pit is in the first layer |
| red key, power key | cap down | the cap is the first layer, the stem and its barbs grow up from it |
| dome | flange down | a solid half sphere: no roof to bridge. At 0.08 mm its steps are small, and they diffuse the light anyway |

The meter has no curve that climbs through the layers except the dome's: every round is in the plane of the bed, every edge that rises is vertical or a 45 degree chamfer. That is why it prints as crisp as it is drawn.

## Settings that matter

- **First layer.** The face is the first layer of the front shell. Level the bed, clean it, and use a smooth or textured PEI sheet to taste: smooth gives a satin face, textured a fine grain. No brim. Keep your printer's elephant's foot compensation on, so the window keeps its size.
- **Ironing** is not needed; if your slicer irons the top of the lid's snaps, turn it off.
- **Seams.** Put the Z seam at the back (the lid) or aligned to a rear corner, away from the face.
- **ASA** needs an enclosure; PETG does not. Either is fine: the meter lives in a pocket, not on a dashboard.
- **The dome.** Translucent PETG at 100 % infill, 0.08 mm, slow outer wall (40 mm/s). A spare is on the plate because the dome is the part you will lose.

## After printing

- Fill the dot on the face and the dot on the dial with a drop of red enamel; wipe the surface flush with a cloth before it dries.
- If you like, fill the engraved name the same way with white or light grey, as on a Leica top plate. Or leave it black: it reads in the light.
- Check that the window takes the module without force: the Waveshare board should drop in face first and stop on the ribs. If your printer runs wide, 0.1 mm of sanding on the window's inner edge is enough.
