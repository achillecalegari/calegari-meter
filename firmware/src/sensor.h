// The AS7341 under the port. It measures without pause on the ESP32's other core, so the screen never waits
// for it and a flash between two frames is not missed.
//
//   light   eight colour channels, clear and near infrared, in two passes; the gain and the integration time
//           follow the light over some 21 stops
//   dome    every few seconds its LED blinks: the translucent dome over the port sends part of that light
//           back, the open port does not
//   flash   the same passes, back to back, at a fixed gain; a pass that reads well above the ones before
//           it caught a flash, and what it reads above them is the flash
//   flicker in Colour, now and then, the chip's own flicker detector (100 or 120 Hz)
#pragma once
#include "ui.h"

namespace sensor {

bool begin();                                  // false: the AS7341 did not answer
void setGains(const float gain[8]);            // the colour calibration, for the lux
void setMode(bool flash, bool colour);         // what the screen is showing
void armFlash();                               // forget the ambient light and wait for a flash
Reading latest();
float probeValue();                            // the last dome probe, for tuning CFG_DOME_MIN

}  // namespace sensor
