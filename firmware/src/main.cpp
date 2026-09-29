// Calegari Meter: the loop that joins the board, the sensor and the screens.
#include <Arduino.h>
#include <Preferences.h>
#include "board.h"
#include "color.h"
#include "config.h"
#include "sensor.h"
#include "ui.h"

static Ui ui;
static Preferences prefs;
static bool sensorOk = false;
static uint32_t saveAt = 0;

static void loadSettings() {
  prefs.begin("meter", true);
  Settings s;
  if (prefs.getBytesLength("s") == sizeof(Settings)) {
    prefs.getBytes("s", &s, sizeof s);
    if (s.version == Settings().version) ui.s = s;
  }
  prefs.end();
}

static void saveSettings() {
  prefs.begin("meter", false);
  prefs.putBytes("s", &ui.s, sizeof ui.s);
  prefs.end();
  saveAt = 0;
}

static void off() {
  saveSettings();
  board::shutdown();
}

// ------------------------------------------------------------------------------------------ inputs

static void readInputs(uint32_t now) {
  int steps = board::dialSteps();
  if (steps) ui.input(IN_DIAL, steps, now);

  // the red key: a press, or held for a second
  static bool redWas = false, longSent = false;
  static uint32_t redSince = 0;
  bool red = board::redDown();
  if (red && !redWas) { redSince = now; longSent = false; }
  if (red && !longSent && now - redSince > 900) { longSent = true; ui.input(IN_RED_LONG, 0, now); }
  if (!red && redWas && !longSent && now - redSince > 25) ui.input(IN_RED, 0, now);
  redWas = red;

  switch (board::powerKey()) {
    case board::PK_SHORT: ui.input(IN_POWER, 0, now); break;
    case board::PK_LONG: off(); break;
    default: break;
  }

  // the screen: a tap, or a swipe across it
  static bool down = false;
  static int x0, y0, x1, y1;
  static uint32_t since;
  board::Touch t = board::touch();
  if (t.down) {
    if (!down) { down = true; x0 = x1 = t.x; y0 = y1 = t.y; since = now; }
    x1 = t.x; y1 = t.y;
  } else if (down && now - since > 20) {
    down = false;
    int dx = x1 - x0, dy = y1 - y0;
    if (abs(dx) > 70 && abs(dx) > 2 * abs(dy)) ui.input(IN_SWIPE, dx < 0 ? 1 : -1, now);   // to the left: next
    else if (abs(dx) < 20 && abs(dy) < 20 && now - since < 500) ui.input(IN_TAP, y0, now);
  }
}

// ------------------------------------------------------------------------------------------ requests

static void serveRequests() {
  Requests &q = ui.req;
  if (q.beep) { q.beep = false; board::beep(); }
  if (q.armFlash) { q.armFlash = false; sensor::armFlash(); }
  if (q.calibrateColour) {
    q.calibrateColour = false;
    Reading r = sensor::latest();
    float g[8];
    bool ok = r.colourValid && color::calibrate(r.channels, ui.s.lampK, g);
    if (ok) {
      for (int i = 0; i < 8; i++) ui.s.colourGain[i] = g[i];
      ui.s.colourCal = 1;
      sensor::setGains(g);
      saveSettings();
    }
    ui.colourCalibrated(ok);
  }
  if (q.bootloader) { q.bootloader = false; saveSettings(); board::bootloader(); }
  if (q.powerOff) { q.powerOff = false; off(); }
}

// ------------------------------------------------------------------------------------------ setup, loop

void setup() {
  Serial.begin(115200);
  bool ok = board::begin();
  loadSettings();
  sensorOk = sensor::begin();
  sensor::setGains(ui.s.colourGain);
  uint32_t now = millis();
  ui.begin(now, true);
  Serial.printf("Calegari Meter %s: board %s, AS7341 %s\n", CFG_VERSION, ok ? "ok" : "FAILED", sensorOk ? "ok" : "FAILED");
}

void loop() {
  uint32_t now = millis();
  readInputs(now);

  sensor::setMode(ui.wantsFlash(), ui.wantsColour());
  Reading r = sensorOk ? sensor::latest() : Reading();
  ui.feed(r);

  board::Battery b = board::battery();
  ui.power.percent = b.percent;
  ui.power.charging = b.charging;

  serveRequests();
  if (ui.settingsDirty()) saveAt = now + 3000;   // settle before writing flash
  if (saveAt && now > saveAt) saveSettings();

  // sleep: dim for the last ten seconds, then off
  uint32_t idle = now - ui.lastInput();
  uint32_t limit = ui.sleepAfterMs();
  int bright = ui.brightness(r.valid ? r.lux : 100);
  if (idle > limit) off();
  if (idle + 10000 > limit) bright = min(bright, 16);

  static uint32_t lastFrame = 0;
  if (now - lastFrame >= 33) {
    lastFrame = now;
    Canvas c{board::framebuffer()};
    ui.draw(c, now);
    board::push(c.fb);
    board::brightness(bright);
  }

  static uint32_t lastLog = 0;
  if (now - lastLog > 1000) {
    lastLog = now;
    Serial.printf("lux %.2f dome %d probe %.2f batt %d%%%s\n", r.lux, r.dome, sensor::probeValue(), b.percent,
                  b.charging ? " charging" : "");
  }
  delay(2);
}
