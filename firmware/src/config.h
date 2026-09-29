// Numbers that come from the case (cad/params.py) and from calibration.
#pragma once

#define CFG_CONE_DEG 22.0f          // half-angle the sensor sees through the front window (cad: cone_half_angle)
#define CFG_DOME_T 0.40f            // share of the light the printed dome passes (calibrated per meter)
#define CFG_HALF_AS_FRACTION 1      // the 1/2 s mark reads "2" as on a Copal dial
#define CFG_VERSION "1.0"

// the board
#define CFG_ROTATE_180 0            // 1 if the screen comes out upside down in your case
#define CFG_ENC_PULSES 4            // encoder edges per detent (EC11: 4)
#define CFG_ENC_REVERSE 0           // 1 if the dial turns the wrong way

// the sensor
#define CFG_LUX_PER_Y 0.0105f       // lux for one basic count of Y (counts per gain per ms); refined by calibration
#define CFG_LUX_PER_CLEAR 0.0060f   // the same for the clear channel, until a steady reading measures it (flash)
#define CFG_DOME_MIN 0.40f          // basic counts of clear its own LED must add to call the dome on
#define CFG_LED_MA 4                // the LED's current for that probe
