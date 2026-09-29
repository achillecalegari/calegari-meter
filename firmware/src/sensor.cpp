#include "sensor.h"
#include <Adafruit_AS7341.h>
#include <Arduino.h>
#include "color.h"
#include "config.h"

namespace sensor {

static Adafruit_AS7341 as;
static SemaphoreHandle_t lock;
static Reading shared;
static float gains[8] = {1, 1, 1, 1, 1, 1, 1, 1};
static volatile bool modeFlash = false, modeColour = false, rearm = false;
static float probe = 0;

// ------------------------------------------------------------------------------------------ ranges
// ATIME 29: 30 steps. ASTEP 599: 1.67 ms a step, 50 ms a pass. The shortest range cuts the step tenfold.

struct Range { as7341_gain_t gain; float g; uint16_t astep; };
static const Range RANGES[] = {
    {AS7341_GAIN_0_5X, 0.5f, 59},   {AS7341_GAIN_0_5X, 0.5f, 599}, {AS7341_GAIN_1X, 1, 599},
    {AS7341_GAIN_2X, 2, 599},       {AS7341_GAIN_4X, 4, 599},      {AS7341_GAIN_8X, 8, 599},
    {AS7341_GAIN_16X, 16, 599},     {AS7341_GAIN_32X, 32, 599},    {AS7341_GAIN_64X, 64, 599},
    {AS7341_GAIN_128X, 128, 599},   {AS7341_GAIN_256X, 256, 599},  {AS7341_GAIN_512X, 512, 599},
    {AS7341_GAIN_512X, 512, 1799},
};
constexpr int RANGE_N = sizeof(RANGES) / sizeof(RANGES[0]);
constexpr uint8_t ATIME = 29;
static int range = 6;

static float tintMs(int i) { return (ATIME + 1) * (RANGES[i].astep + 1) * 2.78e-3f; }
static float fullScale(int i) { return fminf(65535.0f, (ATIME + 1) * (RANGES[i].astep + 1.0f)); }

static void setRange(int i) {
  range = i < 0 ? 0 : (i >= RANGE_N ? RANGE_N - 1 : i);
  as.setASTEP(RANGES[range].astep);
  as.setGain(RANGES[range].gain);
}

// raw: F1 F2 F3 F4 clear NIR (first pass), F5 F6 F7 F8 clear NIR (second pass)
struct Pass {
  uint16_t raw[12];
  float ch[8], clear[2];
  bool sat;
};

static bool measure(Pass &p) {
  if (!as.readAllChannels(p.raw)) return false;
  float k = 1.0f / (RANGES[range].g * tintMs(range));   // basic counts: per unit of gain, per ms
  static const int IDX[8] = {0, 1, 2, 3, 6, 7, 8, 9};
  for (int i = 0; i < 8; i++) p.ch[i] = p.raw[IDX[i]] * k;
  p.clear[0] = p.raw[4] * k;
  p.clear[1] = p.raw[10] * k;
  float fs = fullScale(range) * 0.96f;
  p.sat = false;
  for (int i = 0; i < 12; i++) if (p.raw[i] >= fs) p.sat = true;
  return true;
}

static void publish(const Reading &r) {
  xSemaphoreTake(lock, portMAX_DELAY);
  uint32_t seq = shared.flashSeq;
  float fl = shared.flashLuxSec, fa = shared.flashAmbient;
  bool fo = shared.flashOver;
  int flick = shared.flicker;
  shared = r;
  if (r.flashSeq == 0) { shared.flashSeq = seq; shared.flashLuxSec = fl; shared.flashAmbient = fa; shared.flashOver = fo; }
  if (r.flicker < 0) shared.flicker = flick;
  xSemaphoreGive(lock);
}

// ------------------------------------------------------------------------------------------ light

static float luxPerClear = CFG_LUX_PER_CLEAR;
static bool domeState = false;
static int domeVotes = 0;
static uint32_t lastProbe = 0, lastFlicker = 0;
static float probeClear = -1;

static void lightCycle() {
  Pass p;
  // follow the light: up to three corrections before a reading is published
  for (int tries = 0; tries < 3; tries++) {
    if (!measure(p)) { Reading r; publish(r); delay(200); return; }
    uint16_t peak = 0;
    for (int i = 0; i < 12; i++) if (i != 5 && i != 11) peak = max(peak, p.raw[i]);
    float fs = fullScale(range);
    if (p.sat && range > 0) { setRange(range - 2); continue; }
    if (peak < fs * 0.08f && range < RANGE_N - 1) { setRange(range + (peak < fs * 0.02f ? 2 : 1)); continue; }
    break;
  }
  Reading r;
  r.valid = true;
  r.over = p.sat && range == 0;
  float y = color::luminance(p.ch, gains);
  r.lux = fmaxf(y * CFG_LUX_PER_Y, 0.0f);
  float clear = 0.5f * (p.clear[0] + p.clear[1]);
  if (clear > 50 && !p.sat) luxPerClear = 0.9f * luxPerClear + 0.1f * (r.lux / clear);   // for the flash
  for (int i = 0; i < 8; i++) r.channels[i] = p.ch[i];
  r.colourValid = !p.sat && y > 0.02f;

  // the dome: the LED's light, if it comes back. It blinks when the light jumps (the dome going on or off
  // changes the reading by a stop or more) and every quarter of a minute; in light so strong that the LED
  // would drown in it, the meter keeps what it knew
  float jump = fabsf(log2f(fmaxf(clear, 1e-3f) / fmaxf(probeClear, 1e-3f)));
  if (!p.sat && (jump > 0.7f || millis() - lastProbe > 15000)) {
    lastProbe = millis();
    probeClear = clear;
    int ma = clear > 20 * CFG_DOME_MIN ? CFG_LED_MA * 5 : CFG_LED_MA;
    as.setLEDCurrent(ma);
    as.enableLED(true);
    Pass q;
    bool ok = measure(q);
    as.enableLED(false);
    if (ok) {
      probe = q.sat ? 1e6f : (0.5f * (q.clear[0] + q.clear[1]) - clear) * CFG_LED_MA / ma;
      bool conclusive = q.sat || CFG_DOME_MIN > 0.02f * clear * CFG_LED_MA / ma;
      bool on = probe > CFG_DOME_MIN;
      if (conclusive) {
        if (on != domeState) { if (++domeVotes >= 2) { domeState = on; domeVotes = 0; } else lastProbe = 0; }
        else domeVotes = 0;
      }
    }
  }
  r.dome = domeState;

  // flicker, now and then, in Colour only (it takes half a second)
  r.flicker = -1;
  if (modeColour && millis() - lastFlicker > 6000) {
    lastFlicker = millis();
    uint16_t hz = as.detectFlickerHz();
    r.flicker = (hz == 100 || hz == 120) ? hz : 0;
    as.setATIME(ATIME);
    setRange(range);
  }
  publish(r);
}

// ------------------------------------------------------------------------------------------ flash

static int flashRange = 1;
static float base = -1;             // the ambient light, clear, basic counts
static float burst = 0;             // a flash being added up, basic count ms
static bool burstOver = false;
static uint32_t flashSeq = 0;
static float burstPeak = 0;

static void flashCycle() {
  if (rearm) { rearm = false; base = -1; burst = 0; burstOver = false; }
  if (range != flashRange) setRange(flashRange);
  Pass p;
  if (!measure(p)) { delay(100); return; }
  float t = tintMs(range);
  for (int k = 0; k < 2; k++) {
    float c = p.clear[k];
    bool sat = p.raw[k ? 10 : 4] >= fullScale(range) * 0.96f;
    if (base < 0) { base = c; continue; }
    float noise = 3.0f / (RANGES[range].g * t) + 0.05f * base;   // 3 counts, or 5 % of the ambient
    float ex = c - base;
    if (ex > 6 * noise || sat) {
      burst += ex * t;            // basic counts over the pass, times its length: the flash's share
      burstOver |= sat;
      burstPeak = fmaxf(burstPeak, p.raw[k ? 10 : 4] / fullScale(range));
    } else {
      if (burst > 0) {            // the flash is over: publish it
        Reading r = latest();
        r.flashSeq = ++flashSeq;
        r.flashLuxSec = burst * 1e-3f * luxPerClear;
        r.flashAmbient = base * luxPerClear;
        r.flashOver = burstOver;
        r.flicker = -1;
        publish(r);
        // the next flash: aim its peak at about 40 % of full scale
        if (burstOver && flashRange > 0) flashRange = max(0, flashRange - 2);
        else if (burstPeak < 0.1f && flashRange < 8) flashRange++;
        burst = 0; burstOver = false; burstPeak = 0;
      }
      base = 0.8f * base + 0.2f * c;
    }
  }
  Reading r = latest();
  r.valid = true;
  r.lux = base * luxPerClear;
  r.flashSeq = 0;
  r.flicker = -1;
  publish(r);
}

// ------------------------------------------------------------------------------------------ the task

static void task(void *) {
  for (;;) {
    if (modeFlash) flashCycle();
    else lightCycle();
    delay(1);
  }
}

bool begin() {
  lock = xSemaphoreCreateMutex();
  if (!as.begin(AS7341_I2CADDR_DEFAULT, &Wire)) return false;
  as.setATIME(ATIME);
  setRange(range);
  as.enableLED(false);
  xTaskCreatePinnedToCore(task, "as7341", 6144, nullptr, 2, nullptr, 0);
  return true;
}

void setGains(const float g[8]) {
  xSemaphoreTake(lock, portMAX_DELAY);
  for (int i = 0; i < 8; i++) gains[i] = g[i];
  xSemaphoreGive(lock);
}

void setMode(bool flash, bool colour) {
  if (flash && !modeFlash) rearm = true;
  modeFlash = flash;
  modeColour = colour;
}

void armFlash() { rearm = true; }

Reading latest() {
  xSemaphoreTake(lock, portMAX_DELAY);
  Reading r = shared;
  xSemaphoreGive(lock);
  return r;
}

float probeValue() { return probe; }

}  // namespace sensor
