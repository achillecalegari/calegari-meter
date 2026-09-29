#include "expo.h"
#include "config.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace expo {

const int16_t ISO[] = {6, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80, 100, 125, 160, 200, 250, 320, 400,
                       500, 640, 800, 1000, 1250, 1600, 2000, 2500, 3200, 4000, 5000, 6400};
const int ISO_N = sizeof(ISO) / sizeof(ISO[0]);

// Kodak's published corrections for Tri-X (1 s: +1 stop, 10 s: +2, 100 s: +3) and Fuji's for Acros II
// (none to 120 s, +1/2 stop to 1000 s). Ilford publishes an exponent for each of its films.
static const float TRIX[][2] = {{0.1f, 0.1f}, {1, 2}, {10, 50}, {100, 1200}};
static const float ACROS[][2] = {{120, 120}, {1000, 1414}};

const Film FILMS[] = {
    {"None", 1.0f, nullptr, 0},
    {"HP5 Plus", 1.31f, nullptr, 0},
    {"FP4 Plus", 1.26f, nullptr, 0},
    {"Delta 100", 1.26f, nullptr, 0},
    {"Delta 400", 1.41f, nullptr, 0},
    {"Pan F Plus", 1.33f, nullptr, 0},
    {"Tri-X 400", 0, TRIX, 4},
    {"Acros II", 0, ACROS, 2},
};
const int FILM_N = sizeof(FILMS) / sizeof(FILMS[0]);

float fnum(int k) { return powf(2.0f, k / 6.0f); }
float seconds(int j) { return powf(2.0f, -j / 3.0f); }

static const char *AP[] = {"1", "1.1", "1.2", "1.4", "1.6", "1.8", "2", "2.2", "2.5", "2.8",
                           "3.2", "3.5", "4", "4.5", "5", "5.6", "6.3", "7.1", "8", "9",
                           "10", "11", "13", "14", "16", "18", "20", "22", "25", "29",
                           "32", "36", "40", "45", "51", "57", "64", "72", "80", "90"};

void apLabel(int k, char *out) {
  if (k < AP_MIN) k = AP_MIN;
  if (k > AP_MAX) k = AP_MAX;
  strcpy(out, AP[k]);
}

// j from 39 (1/8000) down to -15 (30 s): the marked values of a modern shutter dial in thirds
static const char *T[] = {"8000", "6400", "5000", "4000", "3200", "2500", "2000", "1600", "1250", "1000",
                          "800",  "640",  "500",  "400",  "320",  "250",  "200",  "160",  "125",  "100",
                          "80",   "60",   "50",   "40",   "30",   "25",   "20",   "15",   "13",   "10",
                          "8",    "6",    "5",    "4",    "0.3",  "0.4",  "0.5",  "0.6",  "0.8",  "1",
                          "1.3",  "1.6",  "2",    "2.5",  "3.2",  "4",    "5",    "6",    "8",    "10",
                          "13",   "15",   "20",   "25",   "30"};

bool isFraction(int j) { return j >= 6 || (j == 3 && CFG_HALF_AS_FRACTION); }

void tLabel(int j, char *out) {
  if (j > T_MAX) j = T_MAX;
  if (j == 3 && CFG_HALF_AS_FRACTION) { strcpy(out, "2"); return; }   // the shutter's "2" is 1/2
  if (j >= -15) { strcpy(out, T[39 - j]); return; }
  float s = seconds(j);
  if (s < 60) snprintf(out, 8, "%d", (int)lroundf(s / 5) * 5);
  else snprintf(out, 8, "%dm", (int)lroundf(s / 60));
}

void secondsLabel(float s, char *main, char *unit, char *rest, char *restUnit) {
  rest[0] = restUnit[0] = 0;
  if (s < 10) {
    snprintf(main, 8, "%.1f", s);
    if (main[strlen(main) - 1] == '0') main[strlen(main) - 2] = 0;
    strcpy(unit, "s");
  } else if (s < 100) {
    snprintf(main, 8, "%d", (int)lroundf(s));
    strcpy(unit, "s");
  } else {
    int total = s < 600 ? (int)lroundf(s / 5) * 5 : (int)lroundf(s / 30) * 30;
    snprintf(main, 8, "%d", total / 60);
    strcpy(unit, "m");
    if (total % 60) { snprintf(rest, 8, "%d", total % 60); strcpy(restUnit, "s"); }
  }
}

// Reflected: behind the meter's cone of half-angle CFG_CONE_DEG, a field of luminance L lights the sensor
// with E = pi L sin^2(theta). EV100 = log2(L S / K), K = 12.5, S = 100.
float ev100Reflected(float lux, float cal) {
  float s = sinf(CFG_CONE_DEG * 3.14159265f / 180.0f);
  float L = lux / (3.14159265f * s * s);
  return log2f(L * 100.0f / 12.5f) + cal;
}

// Incident: EV100 = log2(E S / C) with C = 330 for a dome; the dome passes CFG_DOME_T of the light
float ev100Incident(float lux, float cal) { return log2f((lux / CFG_DOME_T) * 100.0f / 330.0f) + cal; }

static float tableLookup(const float (*t)[2], int n, float x) {
  if (x <= t[0][0]) return x;
  int i = 1;
  while (i < n - 1 && x > t[i][0]) i++;
  float lx0 = logf(t[i - 1][0]), lx1 = logf(t[i][0]), ly0 = logf(t[i - 1][1]), ly1 = logf(t[i][1]);
  return expf(ly0 + (logf(x) - lx0) * (ly1 - ly0) / (lx1 - lx0));
}

float correctReciprocity(const Film &f, float t) {
  if (f.table) return tableLookup(f.table, f.n, t);
  return t > 1.0f ? powf(t, f.p) : t;
}

float inverseReciprocity(const Film &f, float t) {
  float lo = 1e-4f, hi = t;
  if (correctReciprocity(f, t) <= t) return t;
  for (int i = 0; i < 40; i++) {
    float m = sqrtf(lo * hi);
    if (correctReciprocity(f, m) < t) lo = m; else hi = m;
  }
  return sqrtf(lo * hi);
}

}  // namespace expo
