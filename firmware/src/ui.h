// What the meter shows and how its controls drive it. No hardware here: the device (main.cpp) and the host
// renderer (tools/host) feed it readings and inputs, and push its canvas.
//
// Controls
//   dial          changes the value with the red line under it; in lists, moves; when editing, changes
//   red key       measures: holds the reading (Light, Colour), arms (Flash), takes a reading (Zone);
//                 in lists, chooses. Held for a second in Zone: clears the readings
//   power key     opens the modes and settings; in lists, goes back. Held: turns the meter off
//   the screen    a swipe across changes mode
#pragma once
#include <stdint.h>
#include "gfx.h"

enum Mode : uint8_t { M_LIGHT, M_FLASH, M_COLOUR, M_ZONE, MODE_N };

struct Settings {
  uint8_t version = 4;
  uint8_t mode = M_LIGHT;
  uint8_t iso = 18;          // index into expo::ISO: 400
  uint8_t priority = 0;      // 0 aperture, 1 shutter
  int8_t comp = 0;           // thirds of a stop, -9 .. 9
  uint8_t film = 0;          // expo::FILMS
  uint8_t fullStops = 1;     // times in whole stops, as a Copal shutter marks them
  uint8_t fastest = 0;       // 1/500, 1/400, 1/250, 1/125
  uint8_t dome = 0;          // 0 automatic, 1 always on, 2 always off
  int8_t calR = 0, calI = 0; // tenths of a stop
  uint8_t balance = 0;       // film balance: 0 daylight 5500 K, 1 tungsten 3200 K
  uint8_t sleep = 1;         // 30 s, 1, 2, 5 min
  uint8_t bright = 0;        // 0 auto, 1..5
  uint8_t sound = 1;
  int8_t ap = 27;            // f/22
  int8_t sh = 21;            // 1/125
  int8_t sync = 21;          // flash sync, 1/125
  int16_t lampK = 2850;      // the lamp used to calibrate colour
  uint8_t colourCal = 0;
  float colourGain[8] = {1, 1, 1, 1, 1, 1, 1, 1};
};

enum Input { IN_DIAL, IN_RED, IN_RED_LONG, IN_POWER, IN_SWIPE, IN_TAP };

struct Reading {
  bool valid = false;        // the sensor answered
  float lux = 0;             // light at the sensor, calibrated to lux
  bool over = false;         // more than it can take
  bool dome = false;         // the dome is on (the LED's light came back)
  // a flash caught: its light integrated over the burst (lux s), and the ambient light around it
  uint32_t flashSeq = 0;
  float flashLuxSec = 0, flashAmbient = 0;
  bool flashOver = false;
  // colour
  bool colourValid = false;
  float channels[8] = {0};
  int flicker = 0;           // 0, 100 or 120 Hz
};

struct Power {
  int percent = 100;
  bool charging = false;
};

// what the device should do for the UI
struct Requests {
  bool calibrateColour = false;
  bool bootloader = false;
  bool armFlash = false;
  bool powerOff = false;
  bool beep = false;
};

class Ui {
 public:
  Settings s;
  Power power;
  Requests req;

  void begin(uint32_t now, bool splash);
  void feed(const Reading &r);
  void input(Input in, int value, uint32_t now);   // value: dial steps, swipe direction, tap y
  void draw(Canvas &c, uint32_t now);
  bool settingsDirty() { bool d = dirty; dirty = false; return d; }
  uint32_t lastInput() const { return lastTouch; }
  uint32_t sleepAfterMs() const;
  int brightness(float ambientLux) const;   // 0..255
  bool wantsColour() const { return (screen == MAIN && s.mode == M_COLOUR) || screen == CAL_COLOUR; }
  bool wantsFlash() const { return screen == MAIN && s.mode == M_FLASH; }
  bool domeOn() const;
  void colourCalibrated(bool ok) { calMsg = ok ? 1 : 2; }

  // for the host renderer
  void setHold(bool h) { hold = h; if (h) held = live; }
  void openChooser(int row) { screen = CHOOSER; chooserRow = row; }
  void openMenu(int row, bool edit);
  void openCal(bool colour) { screen = colour ? CAL_COLOUR : CAL_LIGHT; calMsg = 0; }
  void addZone(float ev100) { zoneAdd(ev100); }
  void snap() { snapNext = true; }

 private:
  enum Screen { SPLASH, MAIN, CHOOSER, MENU, CAL_COLOUR, CAL_LIGHT } screen = MAIN;
  Reading live, held;
  bool hold = false, dirty = false, editing = false, snapNext = true;
  int menuRow = 0, menuTop = 0, chooserRow = 0, calMsg = 0;
  float calRef = 12.0f;          // the EV the other meter gave, for the light calibration
  uint32_t t0 = 0, lastTouch = 0;
  float scalePos = 0;
  int zoneN = 0;
  float zoneLo = 0, zoneHi = 0;
  uint32_t lastFlashSeq = 0;
  bool flashFresh = false;

  const Reading &r() const { return hold ? held : live; }
  float ev100(float lux, bool incident) const;

  struct Result {
    bool valid, over, under, recip;
    float ev100, ev, exact, secs;
    int shown;
  };
  Result light() const;
  void zoneAdd(float ev);
  void stepSet(int dir);
  void menuStep(int row, int dir);
  void menuValue(int row, char *out, int n) const;
  void calibrateLight();

  void drawSplash(Canvas &c, uint32_t now);
  void drawStatus(Canvas &c, uint32_t now, const char *label);
  void drawLight(Canvas &c, uint32_t now);
  void drawFlash(Canvas &c, uint32_t now);
  void drawColour(Canvas &c, uint32_t now);
  void drawZone(Canvas &c, uint32_t now);
  void drawChooser(Canvas &c);
  void drawMenu(Canvas &c);
  void drawCalColour(Canvas &c);
  void drawCalLight(Canvas &c);
  void drawScale(Canvas &c, float exact, bool apertures, int y0);
  void drawBig(Canvas &c, int yb, const char *pre, const char *num, const char *suf, RGB col);
  void drawSet(Canvas &c, int ys, bool aperture, int value, const char *lead = nullptr);
  void drawHint(Canvas &c, int y, const char *a, const char *b = nullptr);
};

extern const char *const MODE_NAMES[];
