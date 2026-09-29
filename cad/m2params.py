"""Calegari Meter: every dimension in one place, millimetres.

The face is the AMOLED; over it a band like a Leica top plate, and on top the dome and the dial.
Axes: X across (+X to the photographer's LEFT as they look at the screen, as on the View 2), Y up,
Z from the face (0) to the back.

Printed for the printer's sake: every visible face lies flat on the bed or on top, every side is a vertical
wall; round corners only in the bed's plane; 45 degree chamfers on the edges, never fillets that climb the
layers. Two shells: the face plate prints face down, the body prints back down; where they meet is a
shadow line.

Bought parts, placed from their makers' CAD (vendor/):
  Waveshare ESP32-S3-Touch-AMOLED-1.8: module (mx, my, mz), glass at mz 0 -> X = -mx, Y = my + MOD_Y, Z = GLASS_Z - mz
    (turned 180 degrees about Y: its USB-C and side keys come to the photographer's right)
  Adafruit AS7341 (4698), lying flat at the top, looking up: (ax, ay, az) -> X = ax - 12.7 + AS_X, Y = az + AS_Y0, Z = T / 2 + 8.89 - ay
  EC11 encoder (bushing M7, 12.5 mm D shaft), shaft up, under the dial
"""

# ------------------------------------------------------------------------------------------ the module
GW, GH, GR = 33.6, 41.2, 4.0          # the module's glass and frame outline, its corner radius
AW, AH = 29.0, 35.3                   # the AMOLED's active area (368 x 448)
MOD_T = 10.9                          # glass to the tallest part on its back
MOD_BACK = 6.67                       # the display block; the PCB behind it is a little smaller
KEYS_MY = (10.5, -10.5)               # its two side keys: power (upper), BOOT (lower): the measuring key
KEY_MZ = -5.5                         # their actuators, below the glass
KEY_OUT = 16.8                        # at the module's side
USB_MY, USB_MZ = 0.0, -6.15           # USB-C, centre

# ------------------------------------------------------------------------------------------ the block
BEZEL = 4.0                           # round the glass, sides and bottom
BAND = 10.5                           # the top plate's band over the glass
W = GW + 2 * BEZEL                    # 41.6
H = BEZEL + GH + BAND                 # 55.7
R_FACE = GR + BEZEL                   # the lower corners, concentric with the glass's
R_TOP = 3.0                           # the top plate's corners: tighter, like a camera's top plate
CH = 0.8                              # the 45 degree chamfer on the face's and the back's edges
SEAM_CH = 0.4                         # each side of the shadow line
WALL = 1.6
FACE_T = 2.0                          # the face wall
SEAM_Z = 17.0                         # the back lid meets the front shell here: a shadow line 4.4 mm from the back
# The glass is 2.5D: flat to 1.5 mm from its edge, then a shoulder that rolls 1 mm down to the full outline.
# It stands proud of the face by GLASS_PROUD, so its rounded edge is all outside; the face meets the shoulder
# where it is already steep, and inside, hidden under the glass, holds it 0.27 mm before its full outline.
GLASS_PROUD = 0.6
GLASS_Z = -GLASS_PROUD                # the glass's top
LIP_DEPTH = GLASS_PROUD               # the window's edge meets the shoulder at the face
GLASS_SHOULDER = [(0.0, 14.78, 18.58), (-0.2, 15.59, 19.39), (-0.4, 16.08, 19.88), (-0.6, 16.45, 20.25),
                  (-1.0, 16.80, 20.60)]   # (depth below the top, half width, half height), from the maker's model
MOD_Y = BEZEL + GH / 2                # module centre, Y
T = 21.4                              # thickness: the AS7341 lies across it under the dome

# ------------------------------------------------------------------------------------------ battery
BAT = (6.0, 31.0, 40.0)               # up to 603040 (about 700 mAh) with an MX1.25 plug: thick, wide, tall
TRAY_T = 0.8                          # a printed tray between the module's parts and the battery
BAT_Z0 = GLASS_Z + MOD_T + 0.2 + TRAY_T

# ------------------------------------------------------------------------------------------ the top
AS_BOARD = (25.4, 17.78)
AS_X = W / 2 - WALL - AS_BOARD[0] / 2 - 0.2      # pushed to the photographer's left: the dial takes the right
AS_Y0 = H - WALL - 4.53 - 0.3          # its underside: its parts just under the top wall
AS_CHIP = (12.7, 8.51, 2.67)          # the AS7341 on its board: x, y, top z
AS_LED = (12.7, 15.87, 2.27)
CONE_HALF = 22.0                      # reflected light: the port's cone each side of the axis
DOME_R = 5.4
DOME_SEAT = (11.0, 1.2)               # the round seat on top the dome's flange presses into: diameter, depth
LED_WIN = (1.4, 4.6)                  # the window over the LED in the seat's floor: diameter, off the port toward the face

ENC_BODY = (11.8, 13.2, 6.6)          # EC11: body x, z (with its tabs), height (along its shaft)
ENC_BUSH = (7.0, 5.0)                 # threaded bushing: diameter, length
ENC_SHAFT = (6.0, 8.0, 4.5)           # D shaft: diameter, length from the body once cut (cut to 8), flat depth
DIAL = (15.0, 7.0)                    # the dial: diameter, height over the top
DIAL_KNURL = (0.6, 0.45, 36)          # groove width, depth, count

# ------------------------------------------------------------------------------------------ side keys
MEASURE = (5.6, 1.4, 7.6)             # the red key: diameter, how far it stands proud, its well's diameter
MEASURE_WELL = 0.8
POWER = (7.0, 2.6, 0.9)               # the power key: pill length, width, how far it stands proud
KEY_TRAVEL = 0.35                     # free travel before the module's key clicks
