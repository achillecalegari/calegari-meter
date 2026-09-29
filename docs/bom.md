# Shopping list

About 60 EUR, most of it the screen board. Prices are indicative, September 2026.

## Bought

| Qty | Part | Notes | About |
|---|---|---|---|
| 1 | **Waveshare ESP32-S3-Touch-AMOLED-1.8** | the version without case. ESP32-S3R8, 16 MB flash, 1.8 inch AMOLED 368 x 448 with capacitive touch, AXP2101 power chip with charger and power key, ES8311 codec and speaker, USB-C. Not the 1.64 or the 2.06: the case is drawn around this one | 33 EUR |
| 1 | **Adafruit AS7341** 10-channel light sensor, product 4698 | the flat board with two STEMMA QT sockets; we solder to its pins, the sockets stay empty | 16 EUR |
| 1 | **EC11 rotary encoder**, 20 detents, 6 mm D shaft, M7 bushing | the common 12 x 12 mm body with three pins on one side and two (the switch) on the other. The shaft is cut to 8 mm above the body | 2 EUR |
| 1 | **LiPo cell 603040**, 3.7 V, 500 to 600 mAh | at most 6.0 x 31 x 40 mm, with a 2-pin **MX1.25** plug (Waveshare sells one to fit). **Check the polarity** against the BAT marks on the board before you plug it in: cells are sold both ways round | 6 EUR |
| 7 | silicone wire, 28 or 30 AWG | about 60 mm each; seven colours help | 1 EUR |

Also: a soldering iron with a fine tip, solder, a pair of flush cutters, a small hacksaw or a rotary tool to cut the encoder's shaft, a drop of red enamel paint (a paint pen or nail polish) and, if you like, a little white for the engraved name.

## Printed

| Qty | Part | Material | Plate |
|---|---|---|---|
| 1 | front shell | black PETG or ASA | 1 |
| 1 | back lid | black PETG or ASA | 1 |
| 1 | battery tray | black PETG or ASA | 1 |
| 1 | dial | black PETG or ASA | 1 |
| 1 | power key | black PETG or ASA | 1 |
| 2 | red key (one spare) | red PETG or ASA | 2 |
| 2 | dome (one spare) | translucent white PETG, often sold as "natural" or "transparent" | 3 |

At most 16 g of black, under 1 g of red, under 2 g of translucent white.

## Why these

- **The Waveshare 1.8** puts screen, processor, charger, power key and USB on one board with its own header of free pins on the back. The only soldering left is to the sensor and the dial. Its AMOLED is black where it is black, so the meter's dark screen does not glow at night, and it is bright enough for the sun.
- **The AS7341** has eight colour channels across the visible spectrum as well as clear and near infrared, a flicker detector and its own LED. One chip gives light, colour and flash.
- **The 603040 cell** is the biggest that fits behind the screen: a day of metering, recharged over the same USB-C that loads the firmware.
