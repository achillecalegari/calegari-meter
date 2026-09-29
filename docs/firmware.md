# Firmware

The firmware is a [PlatformIO](https://platformio.org) project in `firmware/`, for the Arduino core 3 on the ESP32-S3 (through the pioarduino platform, which PlatformIO downloads by itself).

## Load it

1. Install PlatformIO: the VS Code extension, or on the command line `pip install platformio`.
2. Connect the meter (or the loose Waveshare board) with a USB-C cable that carries data.
3. From the `firmware` folder:

```bash
pio run -t upload
```

The first build downloads the toolchain and the libraries, a few minutes. If the upload does not find the port, put the board into its download mode: hold the **red key** (it presses BOOT), plug the cable in, let go, and run the command again. Once the meter runs, **Settings > Firmware > Update** does the same without the key; unplug and replug the cable after the upload to restart it.

To watch what the sensor reads (useful for [calibration](calibration.md)):

```bash
pio device monitor
```

Once a second it prints the lux, the dome's state and its last probe, and the battery.

## What is in it

| File | What it does |
|---|---|
| `main.cpp` | the loop: inputs, the sensor's readings to the screens, requests back to the board, settings kept in flash, sleep |
| `board.cpp` | the Waveshare board: AMOLED over QSPI (Arduino_GFX, SH8601), touch (FT3168, read directly), power chip (AXP2101: battery, charging, power key, off), speaker (ES8311 over I2S), the red key on GPIO0, the dial's encoder on GPIO17 and 18 |
| `sensor.cpp` | the AS7341 on its own core: gain and integration time following the light, lux from the eight channels, the dome probe, the flash detector, flicker |
| `ui.cpp` | every screen and every control: no hardware in it |
| `expo.cpp` | apertures in thirds, times in thirds or whole stops, EV, reciprocity |
| `color.cpp`, `color_model.h` | eight channels to CIE XYZ, colour temperature (McCamy), tint (Duv), the Wratten filter; the matrix is made by `tools/colorgen.py` |
| `gfx.cpp`, `fonts.cpp` | a small canvas in RGB565, anti-aliased text in Inter; the fonts are made by `tools/fontgen.py` |
| `config.h` | the numbers you may need to change |

## config.h

| Setting | Default | When to change it |
|---|---|---|
| `CFG_ROTATE_180` | 0 | the screen is upside down in the case |
| `CFG_ENC_REVERSE` | 0 | the dial turns the wrong way (or swap A and B) |
| `CFG_ENC_PULSES` | 4 | the dial moves two values per click (set 2) or one every two clicks |
| `CFG_LUX_PER_Y` | 0.0105 | the sensor's lux per count. The light calibration corrects up to 8 stops on top of it; if you need more, change it here |
| `CFG_DOME_T` | 0.40 | the share of light the printed dome passes; the incident calibration takes care of the rest |
| `CFG_DOME_MIN` | 0.40 | how much of its LED's light must come back to call the dome on. Set it between the two probe values the monitor prints with the dome off and on |
| `CFG_CONE_DEG` | 22 | the half angle of the port; follows the CAD |

## The screens on a computer

`ui.cpp` draws into memory and knows nothing of the board, so the same code runs on a Mac or a PC. `tools/host/host.cpp` feeds it seventeen made-up situations and writes each screen to `out/ui/`:

```bash
c++ -O2 -std=c++17 -o out/host tools/host/host.cpp firmware/src/{ui,gfx,expo,fonts,color}.cpp && ./out/host
```

Every picture of the screen in these pages comes from there. Change a screen, run it, look.
