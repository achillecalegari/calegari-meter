#include "color.h"
#include <math.h>
#include "color_model.h"

namespace color {

void blackbody(float kelvin, float out[8]) {
  for (int i = 0; i < 8; i++) out[i] = 0;
  float sigma[8];
  for (int i = 0; i < 8; i++) sigma[i] = CH_FWHM[i] / 2.3548f;
  for (float l = 380; l <= 780; l += 5) {
    double lm = l * 1e-9;
    double b = 1.0 / (pow(lm, 5) * (exp(1.4388e-2 / (lm * kelvin)) - 1.0));
    for (int i = 0; i < 8; i++) {
      float d = (l - CH_NM[i]) / sigma[i];
      out[i] += (float)(b * exp(-0.5 * d * d));
    }
  }
  float m = 0;
  for (int i = 0; i < 8; i++) m = fmaxf(m, out[i]);
  for (int i = 0; i < 8; i++) out[i] /= m;
}

bool calibrate(const float ch[8], float kelvin, float g[8]) {
  float ref[8];
  blackbody(kelvin, ref);
  float mean = 0;
  for (int i = 0; i < 8; i++) {
    if (ch[i] <= 0) return false;
    g[i] = ref[i] / ch[i];
    mean += g[i];
  }
  mean /= 8;
  for (int i = 0; i < 8; i++) g[i] /= mean;
  return true;
}

float luminance(const float ch[8], const float gain[8]) {
  float Y = 0;
  for (int i = 0; i < 8; i++) Y += CH_TO_XYZ[1][i] * ch[i] * gain[i];
  return Y;
}

// the Planckian locus in CIE 1960 uv (Krystek's approximation, 1000..15000 K)
static void planckUV(float t, float *u, float *v) {
  *u = (0.860117757f + 1.54118254e-4f * t + 1.28641212e-7f * t * t) / (1 + 8.42420235e-4f * t + 7.08145163e-7f * t * t);
  *v = (0.317398726f + 4.22806245e-5f * t + 4.20481691e-8f * t * t) / (1 - 2.89741816e-5f * t + 1.61456053e-7f * t * t);
}

Result fromChannels(const float ch[8], const float gain[8]) {
  Result r;
  float X = 0, Y = 0, Z = 0;
  for (int i = 0; i < 8; i++) {
    float c = ch[i] * gain[i];
    X += CH_TO_XYZ[0][i] * c;
    Y += CH_TO_XYZ[1][i] * c;
    Z += CH_TO_XYZ[2][i] * c;
  }
  float s = X + Y + Z;
  if (s <= 0 || Y <= 0) return r;
  r.x = X / s;
  r.y = Y / s;
  float n = (r.x - 0.3320f) / (0.1858f - r.y);
  r.cct = 449 * n * n * n + 3525 * n * n + 6823.3f * n + 5520.33f;
  if (!(r.cct > 1500 && r.cct < 20000)) return r;
  float d = -2 * r.x + 12 * r.y + 3;
  float u = 4 * r.x / d, v = 6 * r.y / d;
  float up, vp;
  planckUV(r.cct, &up, &vp);
  r.duv = sqrtf((u - up) * (u - up) + (v - vp) * (v - vp)) * (v > vp ? 1 : -1);
  r.valid = true;
  return r;
}

// Kodak Wratten light balancing and conversion filters, mired shift
static const Filter FILTERS[] = {
    {"80A", -131}, {"80B", -112}, {"80C", -81}, {"80D", -56}, {"82C", -45}, {"82B", -32}, {"82A", -21},
    {"82", -10},   {"None", 0},   {"81", 10},   {"81A", 18},  {"81B", 27},  {"81C", 35},  {"81D", 42},
    {"81EF", 52},  {"85C", 81},   {"85", 112},  {"85B", 131}};

const Filter &suggest(float cct, float target, int *residual, int *shift) {
  float need = 1e6f / target - 1e6f / cct;
  *shift = (int)lroundf(need);
  int best = 0;
  for (int i = 1; i < (int)(sizeof(FILTERS) / sizeof(FILTERS[0])); i++)
    if (fabsf(FILTERS[i].mired - need) < fabsf(FILTERS[best].mired - need)) best = i;
  *residual = (int)lroundf(need - FILTERS[best].mired);
  return FILTERS[best];
}

}  // namespace color
