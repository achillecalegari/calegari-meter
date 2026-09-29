# Design notes

## The object first

The first drafts were boxes around a board, and they looked it. The meter was redrawn from the outside in, with a few rules taken from the objects it wants to stand next to: a Leica M, a Braun calculator, a Muji notebook, the first iPod shuffle, and the Calegari View 2 it goes with.

1. **One block.** A single rectangle in plan, 41.6 by 55.7 mm, the proportion of the screen plus a band. Nothing sticks out of the sides but two keys; on top only the dome and the dial, the two things you touch on top.
2. **The face is the screen.** The AMOLED fills the face to a 4 mm bezel all round, and the band above it is the meter's top plate, with the name on the left and the red dot on the right, as on a Leica. No empty forehead: the sensor that needs room above the screen lies flat across the top, under the dome, where a light meter's sensor has always been.
3. **Two radii.** The foot has 8 mm corners, soft in the hand; the top has 3 mm corners, square like a top plate. A body that is soft at the bottom and crisp at the top reads as something that stands upright.
4. **The glass stands proud.** The Waveshare's glass has a 2.5D edge. Sunk in a window, that edge would make a step inside the rim; instead the glass stands 0.6 mm proud of the face, its roll all outside, and the face meets it where the roll is already steep. A black glass pebble set in a black block.
5. **Shadow lines, not seams.** The case is two shells. Where they meet, both edges are chamfered, and the joint becomes a fine groove all round, 17 mm from the face, like the line between a camera's top plate and its body.
6. **Black, and one red.** Black body, black glass, one red dot on the face, one on the dial, one red key. Red means: this is the one you press. The screens follow: black, a warm white, a grey for what matters less, and red for the dot, the line under the value you set, and warnings.
7. **Knurled, like the View.** The dial is the View 2's knob in small: a flat top, a 45 degree chamfer, 36 vertical grooves.

## Made for the printer

Every rule for the form is also a rule for printing it.

- **Every round in the plane of the bed.** The corners are rounds in plan, the sides vertical. Nothing curves up through the layers, where a printer can only draw steps. The one exception, the dome, prints at 0.08 mm, and it is a diffuser: its steps help.
- **Chamfers, not fillets.** Every edge that rises from the bed is a 45 degree chamfer: it prints without support and it catches the light as a thin line.
- **The best surface where it shows.** The face is the first layer of the front shell, the back the first layer of the lid. Both are as flat as the bed.
- **Snaps, not screws.** The keys have barbed stems that spring past the wall, the lid has four snaps, the encoder's bushing cuts its own thread in the top wall, the dome and the dial press on.

## The electronics

The meter was first drawn around a small ESP32 board, a separate screen and a coin cell. Everything that makes it pleasant (a sharp screen that is truly black, a battery that lasts, USB charging, sound) meant more boards and more wires. The Waveshare ESP32-S3-Touch-AMOLED-1.8 has all of it on one board behind its glass, with a row of free pins on the back. The meter's shape follows the glass, not the board: the board hides entirely behind the screen.

The AS7341 is the other half: eleven channels, so one chip gives light, colour and flash. It lies across the top under a port. The port is the meter's optics: its rim, 2.56 mm above the chip, is the stop that gives the reflected reading its 22 degree half angle. The dome over it is a solid half sphere of translucent PETG: incident light and a diffuser for colour.

Knowing whether the dome is on usually takes a switch. Here the sensor's own LED does it: a small window above the LED, hidden under the dome's flange, lets its light into the flange, and the flange carries some of it round to the port. No light back, no dome.

## The screens

The type is Inter, in five sizes, drawn with 16 levels of grey so the edges are smooth on the AMOLED. The answer is always the biggest thing on the screen, set like the numbers on a shutter dial: "1/" small, "30" large. The value the dial changes has a red line under it, and turning the dial is all you do to change it. At the foot, a scale slides like a real dial to the answer.
