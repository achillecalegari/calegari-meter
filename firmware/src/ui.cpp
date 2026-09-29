#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "config.h"
#include "expo.h"

using namespace expo;

// ------------------------------------------------------------------------------------------ tables

static const int FASTEST_J[] = {27, 26, 24, 21};   // 1/500, 1/400, 1/250, 1/125
static const char *FASTEST_L[] = {"1/500", "1/400", "1/250", "1/125"};
static const uint32_t SLEEP_MS[] = {30000, 60000, 120000, 300000};
static const char *SLEEP_L[] = {"30 s", "1 min", "2 min", "5 min"};
static const int AP_LENS_MIN = 12, AP_LENS_MAX = 36;   // f/4 .. f/64
static const int SH_SLOWEST = -27;                     // 8 min, set by hand in shutter priority
static const int BALANCE_K[] = {5500, 3200};
static const char *DOME_L[] = {"Automatic", "Always on", "Always off"};

const char *const MODE_NAMES[] = {"Light", "Flash", "Colour", "Zone"};
static const char *MODE_HINT[] = {"Reflected; incident with the dome", "Fire it, read the aperture",
                                  "Kelvin and the filter", "Shadows, highlights, N" G_PLUSMINUS};

enum Row { R_ISO, R_PRIORITY, R_COMP, R_FILM, R_STEPS, R_FASTEST, R_DOME, R_BALANCE, R_CAL_LIGHT, R_CAL_COLOUR,
           R_SLEEP, R_BRIGHT, R_SOUND, R_UPDATE, R_BATTERY, R_VERSION, ROW_N };
static const char *ROW_LABEL[] = {"ISO", "Priority", "Compensation", "Film", "Shutter", "Fastest speed", "Dome",
                                  "Film balance", "Calibrate light", "Calibrate colour", "Sleep", "Brightness",
                                  "Sound", "Firmware", "Battery", "Version"};
static const int ROW_H = 62, ROWS = 6;

// layout
static const int MX = 24;                  // side margin
static const int Y_STATUS = 32;            // status line's baseline
static const int Y_BIG = 236;              // the answer's baseline
static const int Y_NOTE = 120;             // a line over the answer (film, warnings)
static const int Y_SET = 318;              // the value the dial sets
static const int Y_INFO = 366;             // EV and the rest
static const int Y_SCALE = 392;            // the dial at the foot

static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

static void thirds(int v, char *out, int n) {
  if (v == 0) { snprintf(out, n, "0"); return; }
  const char *sign = v > 0 ? "+" : G_MINUS;
  int a = v > 0 ? v : -v, whole = a / 3, rest = a % 3;
  const char *frac = rest == 1 ? G_THIRD : rest == 2 ? G_TWOTHIRDS : "";
  if (whole) snprintf(out, n, "%s%d%s", sign, whole, frac);
  else snprintf(out, n, "%s%s", sign, frac);
}

static void tenths(float v, char *out, int n, bool plus = false) {
  int t = (int)lroundf(v * 10);
  snprintf(out, n, "%s%d.%d", t < 0 ? G_MINUS : (plus && t > 0 ? "+" : ""), (t < 0 ? -t : t) / 10, (t < 0 ? -t : t) % 10);
}

// a black body's colour on screen, for the colour bar (Tanner Helland's fit)
static RGB kelvinRGB(float k) {
  float t = k / 100.0f, r, g, b;
  if (t <= 66) { r = 255; g = 99.47f * logf(t) - 161.12f; }
  else { r = 329.7f * powf(t - 60, -0.1332f); g = 288.12f * powf(t - 60, -0.0755f); }
  if (t >= 66) b = 255;
  else if (t <= 19) b = 0;
  else b = 138.52f * logf(t - 10) - 305.04f;
  auto c = [](float v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
  return RGB{c(r), c(g), c(b)};
}

// ------------------------------------------------------------------------------------------ state

void Ui::begin(uint32_t now, bool splash) {
  t0 = lastTouch = now;
  screen = splash ? SPLASH : MAIN;
  snapNext = true;
}

void Ui::feed(const Reading &rd) {
  live = rd;
  if (rd.flashSeq != lastFlashSeq) {
    lastFlashSeq = rd.flashSeq;
    flashFresh = true;
    held = rd;
    if (s.sound) req.beep = true;
  }
}

bool Ui::domeOn() const {
  if (s.dome == 1) return true;
  if (s.dome == 2) return false;
  return r().dome;
}

uint32_t Ui::sleepAfterMs() const { return SLEEP_MS[s.sleep % 4]; }

int Ui::brightness(float ambientLux) const {
  if (s.bright) return 40 + (s.bright - 1) * 53;
  float t = (log2f(fmaxf(ambientLux, 0.01f)) + 2.0f) / 14.0f;   // dim in the dark, full in daylight
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  return (int)(14 + t * 241);
}

float Ui::ev100(float lux, bool incident) const {
  return incident ? ev100Incident(lux, s.calI / 10.0f) : ev100Reflected(lux, s.calR / 10.0f);
}

Ui::Result Ui::light() const {
  Result o{};
  const Reading &rd = r();
  if (!rd.valid || rd.lux <= 0.002f) return o;
  o.valid = true;
  o.ev100 = ev100(rd.lux, domeOn());
  o.ev = o.ev100 + log2f(ISO[s.iso] / 100.0f) - s.comp / 3.0f;
  const Film &f = FILMS[s.film];
  if (s.priority == 0) {
    float tm = powf(2.0f, -(o.ev - s.ap / 3.0f));
    float tg = correctReciprocity(f, tm);
    o.recip = s.film && tg > tm * 1.05f;
    o.exact = -3.0f * log2f(tg);
    o.secs = tg;
    int j = s.fullStops ? 3 * (int)lroundf(o.exact / 3.0f) : (int)lroundf(o.exact);
    if (j > FASTEST_J[s.fastest]) { j = FASTEST_J[s.fastest]; o.over = true; }
    if (tg > 3600.0f) o.under = true;
    o.shown = j;
  } else {
    float tg = seconds(s.sh);
    float te = inverseReciprocity(f, tg);
    o.recip = s.film && tg > te * 1.05f;
    o.exact = 3.0f * (o.ev + log2f(te));
    int k = (int)lroundf(o.exact);
    if (k > AP_LENS_MAX) { k = AP_LENS_MAX; o.over = true; }
    if (k < AP_LENS_MIN) { k = AP_LENS_MIN; o.under = true; }
    o.shown = k;
  }
  if (rd.over) o.over = true;
  return o;
}

void Ui::zoneAdd(float ev) {
  if (zoneN == 0) zoneLo = zoneHi = ev;
  zoneLo = fminf(zoneLo, ev);
  zoneHi = fmaxf(zoneHi, ev);
  zoneN++;
}

void Ui::stepSet(int dir) {
  if (s.mode == M_FLASH) {
    int j = 3 * (int)lroundf(s.sync / 3.0f) + dir * 3;
    if (j <= FASTEST_J[s.fastest] && j >= 0) s.sync = j;
  } else if (s.mode == M_COLOUR) {
    s.balance = dir > 0 ? 1 : 0;
  } else if (s.mode == M_ZONE || s.priority == 0) {
    int k = s.ap + dir;
    if (k >= AP_LENS_MIN && k <= AP_LENS_MAX) s.ap = k;
  } else {
    int step = s.fullStops ? 3 : 1;
    int j = s.fullStops ? 3 * (int)lroundf(s.sh / 3.0f) : s.sh;
    j -= dir * step;           // turning up gives more light: a slower time
    if (j <= FASTEST_J[s.fastest] && j >= SH_SLOWEST) s.sh = j;
  }
  dirty = true;
}

void Ui::openMenu(int row, bool edit) {
  screen = MENU;
  menuRow = row;
  editing = edit;
  menuTop = clampi(row - (ROWS - 2), 0, ROW_N - ROWS);
}

void Ui::calibrateLight() {
  // the meter should have read calRef: shift the calibration of the current metering by the difference
  const Reading &rd = live;
  if (!rd.valid) { calMsg = 2; return; }
  bool inc = domeOn();
  float now = ev100(rd.lux, inc);
  int delta = (int)lroundf((calRef - now) * 10);
  if (inc) s.calI = clampi(s.calI + delta, -80, 80);
  else s.calR = clampi(s.calR + delta, -80, 80);
  calMsg = 1;
  dirty = true;
}

void Ui::input(Input in, int v, uint32_t now) {
  lastTouch = now;
  if (screen == SPLASH) { screen = MAIN; return; }
  switch (screen) {
    case MAIN:
      if (in == IN_DIAL) { for (int i = 0; i < abs(v); i++) stepSet(v > 0 ? 1 : -1); return; }
      if (in == IN_SWIPE) { s.mode = (s.mode + (v > 0 ? 1 : MODE_N - 1)) % MODE_N; hold = false; snapNext = true; dirty = true;
                            if (s.mode == M_FLASH) { flashFresh = false; req.armFlash = true; } return; }
      if (in == IN_POWER) { screen = CHOOSER; chooserRow = s.mode; return; }
      if (in == IN_RED_LONG && s.mode == M_ZONE) { zoneN = 0; return; }
      if (in == IN_RED || in == IN_TAP) {
        if (s.mode == M_ZONE) { if (live.valid) zoneAdd(ev100(live.lux, domeOn())); }
        else if (s.mode == M_FLASH) { flashFresh = false; req.armFlash = true; }
        else { hold = !hold; if (hold) held = live; }
        if (s.sound && in == IN_RED) req.beep = true;
      }
      return;
    case CHOOSER:
      if (in == IN_DIAL) { chooserRow = clampi(chooserRow + v, 0, MODE_N); return; }
      if (in == IN_TAP) { int row = (v - 96) / 64; if (row >= 0 && row <= MODE_N) chooserRow = row; else return; }
      if (in == IN_POWER) { screen = MAIN; return; }
      if (in == IN_RED || in == IN_TAP) {
        if (chooserRow == MODE_N) { openMenu(0, false); return; }
        s.mode = chooserRow;
        hold = false;
        snapNext = true;
        dirty = true;
        screen = MAIN;
        if (s.mode == M_FLASH) { flashFresh = false; req.armFlash = true; }
      }
      return;
    case MENU:
      if (in == IN_POWER) { if (editing) editing = false; else { screen = MAIN; snapNext = true; } return; }
      if (in == IN_DIAL) {
        if (editing) { for (int i = 0; i < abs(v); i++) menuStep(menuRow, v > 0 ? 1 : -1); dirty = true; return; }
        menuRow = clampi(menuRow + v, 0, ROW_N - 1);
        if (menuRow < menuTop) menuTop = menuRow;
        if (menuRow >= menuTop + ROWS) menuTop = menuRow - ROWS + 1;
        return;
      }
      if (in == IN_TAP) { int row = menuTop + (v - 70) / ROW_H; if (row < 0 || row >= ROW_N) return; menuRow = row; }
      if (in == IN_RED || in == IN_TAP) {
        if (menuRow == R_BATTERY || menuRow == R_VERSION) return;
        if (menuRow == R_CAL_COLOUR) { screen = CAL_COLOUR; calMsg = 0; return; }
        if (menuRow == R_CAL_LIGHT) { screen = CAL_LIGHT; calMsg = 0; return; }
        if (menuRow == R_UPDATE) { req.bootloader = true; return; }
        editing = !editing;
      }
      return;
    case CAL_COLOUR:
      if (in == IN_POWER) { screen = MENU; return; }
      if (in == IN_DIAL) { s.lampK = clampi(s.lampK + v * 50, 2000, 7000); dirty = true; return; }
      if (in == IN_RED) req.calibrateColour = true;
      return;
    case CAL_LIGHT:
      if (in == IN_POWER) { screen = MENU; return; }
      if (in == IN_DIAL) { calRef = fminf(fmaxf(calRef + v * 0.1f, -4.0f), 20.0f); return; }
      if (in == IN_RED) calibrateLight();
      return;
    default: return;
  }
}

void Ui::menuStep(int row, int d) {
  switch (row) {
    case R_ISO: s.iso = clampi(s.iso + d, 0, ISO_N - 1); break;
    case R_PRIORITY: s.priority ^= 1; snapNext = true; break;
    case R_COMP: s.comp = clampi(s.comp + d, -9, 9); break;
    case R_FILM: s.film = (s.film + d + FILM_N) % FILM_N; break;
    case R_STEPS: s.fullStops ^= 1; break;
    case R_FASTEST: s.fastest = clampi(s.fastest - d, 0, 3); break;
    case R_DOME: s.dome = (s.dome + d + 3) % 3; break;
    case R_BALANCE: s.balance ^= 1; break;
    case R_SLEEP: s.sleep = clampi(s.sleep + d, 0, 3); break;
    case R_BRIGHT: s.bright = clampi(s.bright + d, 0, 5); break;
    case R_SOUND: s.sound ^= 1; break;
  }
}

void Ui::menuValue(int row, char *o, int n) const {
  switch (row) {
    case R_ISO: snprintf(o, n, "%d", ISO[s.iso]); break;
    case R_PRIORITY: snprintf(o, n, "%s", s.priority ? "Shutter" : "Aperture"); break;
    case R_COMP: thirds(s.comp, o, n); break;
    case R_FILM: snprintf(o, n, "%s", s.film ? FILMS[s.film].name : "No correction"); break;
    case R_STEPS: snprintf(o, n, "%s", s.fullStops ? "Whole stops" : "Thirds"); break;
    case R_FASTEST: snprintf(o, n, "%s", FASTEST_L[s.fastest]); break;
    case R_DOME: snprintf(o, n, "%s", DOME_L[s.dome]); break;
    case R_BALANCE: snprintf(o, n, "%s", s.balance ? "Tungsten, 3200 K" : "Daylight, 5500 K"); break;
    case R_CAL_LIGHT: {
      char a[8], b[8];
      tenths(s.calR / 10.0f, a, 8, true);
      tenths(s.calI / 10.0f, b, 8, true);
      snprintf(o, n, "%s  %s", a, b);
      break;
    }
    case R_CAL_COLOUR: snprintf(o, n, "%s", s.colourCal ? "Done" : "Not yet"); break;
    case R_SLEEP: snprintf(o, n, "%s", SLEEP_L[s.sleep]); break;
    case R_BRIGHT: if (s.bright) snprintf(o, n, "%d", s.bright); else snprintf(o, n, "Automatic"); break;
    case R_SOUND: snprintf(o, n, "%s", s.sound ? "On" : "Off"); break;
    case R_UPDATE: snprintf(o, n, "Update"); break;
    case R_BATTERY: snprintf(o, n, power.charging ? "%d%% " G_MIDDOT " charging" : "%d%%", power.percent); break;
    case R_VERSION: snprintf(o, n, "%s", CFG_VERSION); break;
  }
}

// ------------------------------------------------------------------------------------------ drawing

void Ui::draw(Canvas &c, uint32_t now) {
  c.clear();
  switch (screen) {
    case SPLASH:
      drawSplash(c, now);
      if (now - t0 > 1500) screen = MAIN;
      break;
    case CHOOSER: drawChooser(c); break;
    case MENU: drawMenu(c); break;
    case CAL_COLOUR: drawCalColour(c); break;
    case CAL_LIGHT: drawCalLight(c); break;
    case MAIN:
      switch (s.mode) {
        case M_FLASH: drawFlash(c, now); break;
        case M_COLOUR: drawColour(c, now); break;
        case M_ZONE: drawZone(c, now); break;
        default: drawLight(c, now);
      }
  }
}

void Ui::drawSplash(Canvas &c, uint32_t now) {
  float t = (now - t0) / 1000.0f;
  int a = (int)(255 * fminf(1.0f, t / 0.4f) * fminf(1.0f, fmaxf(0.0f, (1.5f - t) / 0.35f)));
  c.disc(W / 2, 176, 6.5f, col::red, a);
  c.text(F_MED, W / 2, 238, "CALEGARI", col::white, CENTER, a, 6);
  c.text(F_SMALL, W / 2, 276, "M E T E R", col::grey, CENTER, a);
}

// the red dot (lit while it reads, a ring on hold), the mode's name, ISO, the battery
void Ui::drawStatus(Canvas &c, uint32_t now, const char *label) {
  const int cy = Y_STATUS - 7;
  if (hold) c.ring(MX + 5, cy, 5.5f, 1.8f, col::grey);
  else {
    float ph = (now % 2400) / 2400.0f;
    c.disc(MX + 5, cy, 5.5f, col::red, 190 + (int)(65 * (0.5f + 0.5f * cosf(ph * 6.2832f))));
  }
  c.text(F_TINY, MX + 20, Y_STATUS, label, col::grey, LEFT, 255, 3);
  // battery, then ISO to its left
  int bx = W - MX - 28, by = Y_STATUS - 14;
  bool low = power.percent <= 15 && !power.charging;
  RGB bc = low ? col::red : col::grey;
  c.fillRect(bx, by, 25, 2, bc); c.fillRect(bx, by + 13, 25, 2, bc);
  c.fillRect(bx, by, 2, 15, bc); c.fillRect(bx + 23, by, 2, 15, bc);
  c.fillRect(bx + 25, by + 4, 3, 7, bc);
  int fill = power.charging ? (int)((now / 350) % 20) : (power.percent * 19 + 50) / 100;
  c.fillRect(bx + 3, by + 3, fill, 9, low ? col::red : col::white);
  char buf[16];
  snprintf(buf, sizeof buf, "%d", ISO[s.iso]);
  int w = c.text(F_TINY, bx - 14, Y_STATUS, buf, col::white, RIGHT);
  c.text(F_TINY, bx - 14 - w - 7, Y_STATUS, "ISO", col::dim, RIGHT, 255, 1);
}

// prefix small, number huge, unit small, all centred
void Ui::drawBig(Canvas &c, int yb, const char *pre, const char *num, const char *suf, RGB colr) {
  int w = (pre[0] ? Canvas::width(F_BIG, pre) + 2 : 0) + Canvas::width(F_HUGE, num) +
          (suf[0] ? 4 + Canvas::width(F_BIG, suf) : 0);
  const Font &f = w > W - 2 * MX ? F_BIG : F_HUGE;   // never wider than the screen
  if (&f == &F_BIG) w = (pre[0] ? Canvas::width(F_BIG, pre) + 2 : 0) + Canvas::width(F_BIG, num) + (suf[0] ? 4 + Canvas::width(F_BIG, suf) : 0);
  int x = (W - w) / 2;
  if (pre[0]) x += c.text(F_BIG, x, yb, pre, col::grey) + 2;
  x += c.text(f, x, yb, num, colr);
  if (suf[0]) c.text(F_BIG, x + 4, yb, suf, col::grey);
}

// the value the dial sets, with its red line
void Ui::drawSet(Canvas &c, int ys, bool aperture, int v, const char *lead) {
  char buf[12];
  int wl = lead ? Canvas::width(F_MED, lead) + 10 : 0;
  if (aperture) {
    apLabel(v, buf);
    int wn = Canvas::width(F_BIG, buf), wp = Canvas::width(F_MED, "f/") + 3;
    int x = (W - wn - wp - wl) / 2;
    if (lead) { c.text(F_MED, x, ys, lead, col::grey); x += wl; }
    c.text(F_MED, x, ys, "f/", col::grey);
    c.text(F_BIG, x + wp, ys, buf, col::white);
    c.fillRect(x + wp, ys + 11, wn, 3, col::red);
  } else {
    tLabel(v, buf);
    bool fr = isFraction(v);
    int wn = Canvas::width(F_BIG, buf), wp = fr ? Canvas::width(F_MED, "1/") + 2 : 0,
        wu = fr ? 0 : Canvas::width(F_MED, "s") + 3;
    int x = (W - wn - wp - wu - wl) / 2;
    if (lead) { c.text(F_MED, x, ys, lead, col::grey); x += wl; }
    if (fr) c.text(F_MED, x, ys, "1/", col::grey);
    c.text(F_BIG, x + wp, ys, buf, col::white);
    if (!fr) c.text(F_MED, x + wp + wn + 3, ys, "s", col::grey);
    c.fillRect(x + wp, ys + 11, wn, 3, col::red);
  }
}

void Ui::drawHint(Canvas &c, int y, const char *a, const char *b) {
  c.text(F_SMALL, W / 2, y, a, col::grey, CENTER);
  if (b) c.text(F_SMALL, W / 2, y + 32, b, col::grey, CENTER);
}

void Ui::drawLight(Canvas &c, uint32_t now) {
  bool inc = domeOn();
  drawStatus(c, now, inc ? "INCIDENT" : "REFLECTED");
  Result o = light();
  char num[16], unit[4], rest[8], ru[4], buf[24];
  RGB big = (o.over || o.under) ? col::red : col::white;
  if (!o.valid) {
    c.text(F_BIG, W / 2, Y_BIG - 30, G_MIDDOT " " G_MIDDOT " " G_MIDDOT, col::dim, CENTER);
    drawHint(c, Y_BIG + 20, "Too dark to read");
  } else if (s.priority == 0) {
    if (o.secs >= 1.0f && !o.over) {
      secondsLabel(o.secs, num, unit, rest, ru);
      if (rest[0]) {
        drawBig(c, Y_BIG, "", num, unit, big);
        snprintf(buf, sizeof buf, "%s%s", rest, ru);
        c.text(F_MED, W / 2, Y_BIG + 36, buf, big, CENTER);
      } else drawBig(c, Y_BIG, "", num, unit, big);
    } else {
      tLabel(o.shown, num);
      if (isFraction(o.shown)) drawBig(c, Y_BIG, "1/", num, "", big);
      else drawBig(c, Y_BIG, "", num, "s", big);
    }
  } else {
    apLabel(o.shown, num);
    drawBig(c, Y_BIG, "f/", num, "", big);
  }
  if (o.valid && (o.over || o.under)) c.text(F_SMALL, W / 2, Y_NOTE, o.over ? "Too bright" : "Too dark", col::red, CENTER);
  else if (o.valid && o.recip) {
    snprintf(buf, sizeof buf, "%s, corrected", FILMS[s.film].name);
    c.text(F_SMALL, W / 2, Y_NOTE, buf, col::grey, CENTER);
  }
  if (s.priority == 0) drawSet(c, Y_SET, true, s.ap);
  else drawSet(c, Y_SET, false, s.sh);
  if (o.valid) {
    tenths(o.ev100, buf, sizeof buf);
    int wv = Canvas::width(F_SMALL, buf), we = Canvas::width(F_SMALL, "EV") + 8;
    int x0 = (W - wv - we) / 2;
    c.text(F_SMALL, x0, Y_INFO, "EV", col::dim);
    c.text(F_SMALL, x0 + we, Y_INFO, buf, hold ? col::white : col::grey);
    if (s.comp) { thirds(s.comp, buf, sizeof buf); c.text(F_SMALL, W - MX, Y_INFO, buf, col::red, RIGHT); }
  }
  drawScale(c, o.valid ? o.exact : (s.priority ? 24 : 21), s.priority == 1, Y_SCALE);
}

void Ui::drawFlash(Canvas &c, uint32_t now) {
  drawStatus(c, now, "FLASH");
  char buf[48];
  if (!flashFresh) {
    float ph = (now % 1600) / 1600.0f;
    c.ring(W / 2, 178, 46 + 7 * sinf(ph * 6.2832f), 3.0f, col::red, 150);
    c.disc(W / 2, 178, 7, col::red);
    drawHint(c, Y_NOTE - 50, "Fire the flash");
    drawSet(c, Y_SET, false, s.sync, "sync");
    drawScale(c, 24, true, Y_SCALE);
    return;
  }
  const Reading &f = held;
  float t = seconds(s.sync);
  float h = f.flashLuxSec + f.flashAmbient * t;         // the flash, and the ambient light while the shutter is open
  float ev = ev100(h, domeOn()) + log2f(ISO[s.iso] / 100.0f) - s.comp / 3.0f;   // as a 1 s exposure of h
  float exact = 3.0f * ev;
  int k = (int)lroundf(exact);
  bool over = f.flashOver || k > AP_MAX, under = k < AP_MIN;
  k = clampi(k, AP_MIN, AP_MAX);
  apLabel(k, buf);
  drawBig(c, Y_BIG, "f/", buf, "", over || under ? col::red : col::white);
  if (over || under) c.text(F_SMALL, W / 2, Y_NOTE, over ? "Too bright" : "Too dark", col::red, CENTER);
  else {
    int tt = (int)lroundf(exact / 3.0f * 10);            // tenths of a stop, so 9.96 tenths is the next stop
    int base = 3 * (int)floorf(tt / 10.0f), tn = tt - base / 3 * 10;
    char a[8];
    apLabel(base, a);
    if (tn) snprintf(buf, sizeof buf, "f/%s and %d tenths", a, tn);
    else snprintf(buf, sizeof buf, "right on f/%s", a);
    c.text(F_SMALL, W / 2, Y_NOTE, buf, col::grey, CENTER);
  }
  drawSet(c, Y_SET, false, s.sync, "sync");
  int share = h > 0 ? (int)lroundf(100 * f.flashLuxSec / h) : 0;
  snprintf(buf, sizeof buf, "%d%% flash " G_MIDDOT " %d%% ambient " G_MIDDOT " %s", share, 100 - share, domeOn() ? "inc." : "refl.");
  c.text(F_SMALL, W / 2, Y_INFO, buf, col::grey, CENTER);
  drawScale(c, exact, true, Y_SCALE);
}

void Ui::drawColour(Canvas &c, uint32_t now) {
  drawStatus(c, now, "COLOUR");
  const Reading &rd = r();
  char buf[32];
  color::Result cr;
  if (rd.colourValid) cr = color::fromChannels(rd.channels, s.colourGain);
  if (!domeOn()) c.text(F_SMALL, W / 2, Y_NOTE - 36, "Put the dome on", col::red, CENTER);
  if (!s.colourCal) c.text(F_TINY, W / 2, Y_NOTE, "Not calibrated", col::redDim, CENTER, 255, 1);
  const int yb = 206;
  if (!cr.valid) {
    c.text(F_BIG, W / 2, yb - 30, G_MIDDOT " " G_MIDDOT " " G_MIDDOT, col::dim, CENTER);
    drawHint(c, yb + 20, rd.colourValid ? "No colour to read" : "Too dark");
  } else {
    int k = (int)lroundf(cr.cct / 10.0f) * 10;
    snprintf(buf, sizeof buf, "%d", k);
    drawBig(c, yb, "", buf, "K", col::white);
  }
  // the bar: 2000 K to 10000 K on a mired scale, the film's balance marked, the light where it falls
  const int bx0 = MX + 8, bx1 = W - MX - 8, by = 236, bh = 10;
  auto xOf = [&](float kel) {
    float m0 = 1e6f / 2000, m1 = 1e6f / 10000, m = 1e6f / kel;
    return bx0 + (int)lroundf((m0 - m) / (m0 - m1) * (bx1 - bx0));
  };
  for (int x = bx0; x <= bx1; x++) {
    float m0 = 1e6f / 2000, m1 = 1e6f / 10000;
    float m = m0 - (x - bx0) * (m0 - m1) / (bx1 - bx0);
    RGB col_ = kelvinRGB(1e6f / m);
    c.fillRect(x, by, 1, bh, Canvas::mix(col::black, col_, 150));
  }
  int xt = xOf(BALANCE_K[s.balance]);
  c.fillRect(xt, by - 5, 1, bh + 10, col::grey);
  if (cr.valid) {
    int xm = xOf(fminf(fmaxf(cr.cct, 2000), 10000));
    c.triangle(xm - 6.0f, by - 9, xm + 6.0f, by - 9, xm, by - 1, col::white);
    c.fillRect(xm, by, 1, bh, col::white);
  }
  c.text(F_TINY, bx0, by + 32, "2000", col::dim);
  c.text(F_TINY, bx1, by + 32, "10000", col::dim, RIGHT);
  if (cr.valid) {
    int residual, shift;
    const color::Filter &fl = color::suggest(cr.cct, BALANCE_K[s.balance], &residual, &shift);
    int wn = Canvas::width(F_BIG, fl.name);
    c.text(F_BIG, W / 2, Y_SET, fl.name, col::white, CENTER);
    c.fillRect((W - wn) / 2, Y_SET + 11, wn, 3, col::red);
    snprintf(buf, sizeof buf, "%s film " G_MIDDOT " %s%d mired", s.balance ? "tungsten" : "daylight",
             shift > 0 ? "+" : (shift < 0 ? G_MINUS : ""), abs(shift));
    c.text(F_SMALL, W / 2, Y_INFO, buf, col::grey, CENTER);
    int tint = (int)lroundf(cr.duv * 1000);
    if (tint) {
      snprintf(buf, sizeof buf, "%s tint %d", tint > 0 ? "Green" : "Magenta", abs(tint));
      c.text(F_SMALL, W / 2, Y_INFO + 32, buf, abs(tint) >= 6 ? col::red : col::grey, CENTER);
    }
  }
  if (rd.flicker) {
    snprintf(buf, sizeof buf, "Flicker at %d Hz", rd.flicker);
    c.text(F_SMALL, W / 2, H - 16, buf, col::red, CENTER);
  }
}

void Ui::drawZone(Canvas &c, uint32_t now) {
  drawStatus(c, now, "ZONE");
  char buf[32], num[16];
  static const char *ROMAN[] = {"0", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"};
  // the zones: true greys, Zone I to X, across the top
  const int zx0 = MX, zw = (W - 2 * MX) / 10, zy = 64, zh = 38;
  for (int z = 1; z <= 10; z++) {
    // reflectance of Zone z, as seen on screen: a stop per zone, V is 18 %
    float refl = 0.18f * powf(2.0f, z - 5.0f);
    float v = powf(fminf(refl, 1.0f), 1 / 2.2f) * 255;
    uint8_t g = (uint8_t)fminf(v, 250);
    c.fillRect(zx0 + (z - 1) * zw, zy, zw, zh, RGB{g, g, (uint8_t)(g * 0.98f)});
    c.text(F_TINY, zx0 + (z - 1) * zw + zw / 2, zy + zh + 24, ROMAN[z], z == 3 || z == 5 || z == 8 ? col::grey : col::dim, CENTER);
  }
  auto mark = [&](float z, RGB colr) {
    float zz = fminf(fmaxf(z, 0.55f), 10.45f);
    int x = zx0 + (int)lroundf((zz - 0.5f) * zw);
    c.triangle(x - 7.0f, zy - 12, x + 7.0f, zy - 12, x, zy - 2, colr);
  };
  if (domeOn()) c.text(F_SMALL, W / 2, H - 16, "Take the dome off", col::red, CENTER);
  if (zoneN == 0) {
    drawHint(c, 200, "Point it at the shadows", "and press the red key,");
    drawHint(c, 264, "then at the highlights");
    drawSet(c, Y_SET + 40, true, s.ap);
    return;
  }
  float range = zoneHi - zoneLo;
  float evExp = zoneLo + log2f(ISO[s.iso] / 100.0f) + 2.0f;     // the shadow on III: two stops less than its reading
  float hiZone = 3.0f + range;
  int dev = (int)lroundf(8.0f - hiZone);                          // development that brings it to VIII
  mark(3, col::white);
  mark(hiZone, col::red);
  char r1[8];
  tenths(range, r1, sizeof r1);
  snprintf(buf, sizeof buf, "%s stops " G_MIDDOT " %d reading%s", r1, zoneN, zoneN > 1 ? "s" : "");
  c.text(F_SMALL, W / 2, 176, buf, col::grey, CENTER);
  if (dev == 0) snprintf(num, sizeof num, "N");
  else snprintf(num, sizeof num, "N%s%d", dev > 0 ? "+" : G_MINUS, abs(dev));
  drawBig(c, 290, "", num, "", abs(dev) > 2 ? col::red : col::white);
  // the exposure that puts the shadow on III, at the aperture the dial sets
  float tv = evExp - s.ap / 3.0f;
  int j = s.fullStops ? 3 * (int)lroundf(tv) : (int)lroundf(3 * tv);
  char t[8], a[8];
  tLabel(j, t);
  apLabel(s.ap, a);
  snprintf(buf, sizeof buf, "%s%s%s", isFraction(j) ? "1/" : "", t, isFraction(j) ? "" : "s");
  int w1 = Canvas::width(F_BIG, buf), w2 = Canvas::width(F_MED, "  at f/"), w3 = Canvas::width(F_BIG, a);
  int x = (W - w1 - w2 - w3) / 2;
  const int ye = 380;
  x += c.text(F_BIG, x, ye, buf, col::white);
  x += c.text(F_MED, x, ye, "  at f/", col::grey);
  c.text(F_BIG, x, ye, a, col::white);
  c.fillRect(x, ye + 11, w3, 3, col::red);
}

void Ui::drawScale(Canvas &c, float target, bool apertures, int y0) {
  const int cx = W / 2;
  const float px = 16.0f;   // pixels per third: 48 per stop
  if (snapNext) { scalePos = target; snapNext = false; }
  scalePos += (target - scalePos) * 0.25f;
  if (fabsf(target - scalePos) < 0.01f) scalePos = target;
  int lo = (int)floorf(scalePos - cx / px) - 1, hi = (int)ceilf(scalePos + cx / px) + 1;
  char buf[12];
  for (int v = lo; v <= hi; v++) {
    float xf = cx + (v - scalePos) * px;
    int xi = (int)lroundf(xf);
    if (xi < 0 || xi >= W) continue;
    float d = fabsf(xf - cx) / cx;
    int a = (int)(255 * fmaxf(0.08f, 1.0f - d * d));
    bool outside = apertures ? (v < AP_LENS_MIN || v > AP_LENS_MAX) : (v > FASTEST_J[s.fastest] || v < T_MIN);
    bool full = ((v % 3) + 3) % 3 == 0;
    RGB tc = outside ? col::redDim : (full ? col::white : col::grey);
    c.fillRect(xi, y0, full ? 2 : 1, full ? 12 : 6, Canvas::mix(col::black, tc, a));
    if (full && !outside) {
      if (!apertures && v >= T_MIN && v <= T_MAX) {
        tLabel(v, buf);
        if (!isFraction(v) && buf[strlen(buf) - 1] != 'm') strcat(buf, "s");
        c.text(F_TINY, xi + 1, y0 + 38, buf, isFraction(v) ? col::white : col::grey, CENTER, a);
      } else if (apertures && v >= AP_MIN && v <= AP_MAX) {
        apLabel(v, buf);
        c.text(F_TINY, xi + 1, y0 + 38, buf, col::white, CENTER, a);
      }
    }
  }
  c.triangle(cx - 7.0f, y0 - 10, cx + 7.0f, y0 - 10, cx, y0 - 2, col::red);
  c.fillRect(cx, y0, 2, 18, col::red);
}

void Ui::drawChooser(Canvas &c) {
  c.text(F_TINY, MX, Y_STATUS, "MODE", col::dim, LEFT, 255, 3);
  for (int i = 0; i <= MODE_N; i++) {
    int y = 128 + i * 64;
    bool sel = i == chooserRow;
    const char *name = i == MODE_N ? "Settings" : MODE_NAMES[i];
    if (sel) c.disc(MX + 6, y - 12, 5.5f, col::red);
    c.text(F_MED, MX + 26, y, name, sel ? col::white : (i == MODE_N ? col::dim : col::grey));
    if (i == s.mode && !sel) c.fillRect(MX + 26, y + 9, 22, 2, col::dim);
  }
  if (chooserRow < MODE_N) c.text(F_TINY, MX + 26, H - 22, MODE_HINT[chooserRow], col::grey);
}

void Ui::drawMenu(Canvas &c) {
  char buf[40];
  c.text(F_TINY, MX, Y_STATUS, "SETTINGS", col::dim, LEFT, 255, 3);
  for (int i = 0; i < ROWS; i++) {
    int row = menuTop + i;
    if (row >= ROW_N) break;
    int y = 70 + i * ROW_H;
    bool sel = row == menuRow;
    if (sel) c.disc(MX - 8, y + 40, 4.5f, col::red);
    c.text(F_TINY, MX + 6, y + 18, ROW_LABEL[row], sel ? col::grey : col::dim, LEFT, 255, 1);
    menuValue(row, buf, sizeof buf);
    int w = c.text(F_MED, MX + 6, y + 52, buf, sel ? col::white : col::grey);
    if (sel && editing) c.fillRect(MX + 6, y + 60, w, 3, col::red);
  }
  int track = ROWS * ROW_H - 16, th = track * ROWS / ROW_N, ty = 78 + (track - th) * menuTop / (ROW_N - ROWS);
  c.fillRect(W - 8, 78, 2, track, col::faint);
  c.fillRect(W - 8, ty, 2, th, col::grey);
}

void Ui::drawCalColour(Canvas &c) {
  char buf[24];
  c.text(F_TINY, MX, Y_STATUS, "CALIBRATE COLOUR", col::dim, LEFT, 255, 3);
  const char *lines[] = {"Put the dome on and hold it", "close to a halogen or", "filament bulb. Set its", "temperature with the dial,", "then press the red key."};
  for (int i = 0; i < 5; i++) c.text(F_SMALL, MX, 96 + i * 34, lines[i], col::grey);
  snprintf(buf, sizeof buf, "%d", s.lampK);
  int wn = Canvas::width(F_BIG, buf), wk = Canvas::width(F_MED, "K") + 6;
  int x = (W - wn - wk) / 2;
  c.text(F_BIG, x, 340, buf, col::white);
  c.text(F_MED, x + wn + 6, 340, "K", col::grey);
  c.fillRect(x, 352, wn, 3, col::red);
  const char *msg = calMsg == 1 ? "Calibrated" : calMsg == 2 ? "Too dim, bring it closer" : "Most bulbs: 2700 to 3000 K";
  c.text(F_SMALL, W / 2, 410, msg, calMsg == 2 ? col::red : (calMsg == 1 ? col::white : col::dim), CENTER);
}

void Ui::drawCalLight(Canvas &c) {
  char buf[24];
  bool inc = domeOn();
  c.text(F_TINY, MX, Y_STATUS, inc ? "CALIBRATE, INCIDENT" : "CALIBRATE, REFLECTED", col::dim, LEFT, 255, 3);
  const char *lines[] = {"Read the same light with", "a meter you trust. Set its", "EV at ISO 100 with the dial,", "then press the red key."};
  for (int i = 0; i < 4; i++) c.text(F_SMALL, MX, 96 + i * 34, lines[i], col::grey);
  if (live.valid) {
    tenths(ev100(live.lux, inc), buf, sizeof buf);
    char t[32];
    snprintf(t, sizeof t, "This meter reads EV %s", buf);
    c.text(F_SMALL, W / 2, 270, t, col::grey, CENTER);
  }
  tenths(calRef, buf, sizeof buf);
  int wn = Canvas::width(F_BIG, buf), we = Canvas::width(F_MED, "EV") + 10;
  int x = (W - wn - we) / 2;
  c.text(F_MED, x, 350, "EV", col::grey);
  c.text(F_BIG, x + we, 350, buf, col::white);
  c.fillRect(x + we, 362, wn, 3, col::red);
  const char *msg = calMsg == 1 ? "Calibrated" : calMsg == 2 ? "No reading, try again" : "";
  c.text(F_SMALL, W / 2, 416, msg, calMsg == 2 ? col::red : col::white, CENTER);
}
