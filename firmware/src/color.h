// Colour temperature from the AS7341's eight visible channels.
//
// Channels -> CIE XYZ with a fixed matrix (color_model.h), XYZ -> xy -> correlated colour temperature
// (McCamy) and the distance from the Planckian locus (Duv: + is green, - is magenta). Each channel's real
// gain, diffuser included, comes from one calibration under a lamp of known temperature.
#pragma once

namespace color {

struct Result {
  bool valid = false;
  float cct = 0, duv = 0, x = 0, y = 0;
};

Result fromChannels(const float ch[8], const float gain[8]);
// Y, in the channels' units: times a constant, lux
float luminance(const float ch[8], const float gain[8]);
// what the eight channels read under a black body of that temperature (the model; normalised)
void blackbody(float kelvin, float out[8]);
// under a lamp of known colour temperature: the gains that make the channels read like that lamp
bool calibrate(const float ch[8], float kelvin, float gainOut[8]);

struct Filter { const char *name; int mired; };
// the Wratten filter that brings `cct` nearest to `target`; residual in mired after the filter
const Filter &suggest(float cct, float target, int *residual, int *shift);

}  // namespace color
