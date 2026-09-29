// Exposure arithmetic, in thirds of a stop.
//
// Apertures: index k, f-number N = 2^(k/6): k 0 is f/1, k 36 is f/64.
// Times: index j, t = 2^(-j/3) seconds: j 0 is 1 s, j 21 is 1/125, j -18 is 64 s.
// A reading is an exposure value at the film's speed, EV = Av + Tv, with Av = k/3 and Tv = j/3.
#pragma once
#include <stdint.h>

namespace expo {

constexpr int AP_MIN = 0, AP_MAX = 39;      // f/1 .. f/90
constexpr int T_MIN = -36, T_MAX = 39;      // 16 min .. 1/8000

extern const int16_t ISO[];                 // 6 .. 6400 in thirds
extern const int ISO_N;

struct Film {
  const char *name;
  float p;                                  // Schwarzschild exponent past 1 s (Ilford: Tc = Tm^p); 0: use the table
  const float (*table)[2];                  // (metered s, corrected s) pairs, log-log interpolated
  int n;
};
extern const Film FILMS[];
extern const int FILM_N;

float fnum(int k);                          // exact f-number
float seconds(int j);                       // exact time

// Labels on the lens and the shutter: "22", "5.6"; "125" (a fraction) or "2" (seconds, see isFraction)
void apLabel(int k, char *out);
void tLabel(int j, char *out);
bool isFraction(int j);
// A time that is not on the dial (reciprocity), in seconds: "13", "2m 40" and so on
void secondsLabel(float s, char *main, char *unit, char *rest, char *restUnit);

// Exposure value at ISO 100 from the sensor's lux: reflected through the meter's cone, or incident
// through the dome. cal is the user's calibration in EV.
float ev100Reflected(float lux, float cal);
float ev100Incident(float lux, float cal);

float correctReciprocity(const Film &f, float t);   // metered -> to give
float inverseReciprocity(const Film &f, float t);   // given -> what the film sees

}  // namespace expo
