// Renders every screen of the meter on a computer, with the firmware's own drawing code: out/ui/*.ppm
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/color.h"
#include "../../firmware/src/expo.h"
#include "../../firmware/src/ui.h"

static uint16_t fb[W * H];

static float luxForEV(float ev100, bool incident) {   // inverse of the firmware's EV, calibration 0
  float lo = 1e-4f, hi = 1e7f;
  for (int i = 0; i < 80; i++) {
    float m = sqrtf(lo * hi);
    float ev = incident ? expo::ev100Incident(m, 0) : expo::ev100Reflected(m, 0);
    if (ev < ev100) lo = m; else hi = m;
  }
  return sqrtf(lo * hi);
}

static void save(Ui &ui, const char *name, uint32_t now = 5000) {
  Canvas c{fb};
  ui.snap();
  ui.draw(c, now);
  char path[256];
  snprintf(path, sizeof path, "out/ui/%s.ppm", name);
  FILE *f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", W, H);
  for (int i = 0; i < W * H; i++) {
    RGB p = Canvas::unpack(fb[i]);
    fputc(p.r, f); fputc(p.g, f); fputc(p.b, f);
  }
  fclose(f);
  printf("%s\n", path);
}

static Reading light(float ev100, bool dome) {
  Reading r;
  r.valid = true;
  r.dome = dome;
  r.lux = luxForEV(ev100, dome);
  return r;
}

static Reading colour(float kelvin, float greenBump, int flicker) {
  Reading r = light(10, true);
  r.colourValid = true;
  color::blackbody(kelvin, r.channels);
  r.channels[3] *= 1 + greenBump;
  r.channels[4] *= 1 + greenBump;
  r.flicker = flicker;
  return r;
}

int main() {
  { Ui u; u.begin(0, false); u.power.percent = 82; u.feed(light(12.2f, false)); save(u, "01_light_reflected"); }
  { Ui u; u.begin(0, false); u.power.percent = 82; u.s.comp = 2; u.s.ap = 24; u.feed(light(9.2f, true)); u.setHold(true); save(u, "02_light_incident_hold"); }
  { Ui u; u.begin(0, false); u.power.percent = 64; u.s.priority = 1; u.s.sh = 18; u.s.iso = 12; u.feed(light(13.6f, false)); save(u, "03_light_shutter"); }
  { Ui u; u.begin(0, false); u.power.percent = 64; u.s.film = 1; u.s.ap = 30; u.feed(light(4.3f, false)); save(u, "04_light_long_hp5"); }
  { Ui u; u.begin(0, false); u.power.percent = 12; u.s.ap = 15; u.feed(light(16.0f, false)); save(u, "05_light_too_bright"); }
  { Ui u; u.begin(0, false); u.s.mode = M_FLASH; u.feed(light(6.0f, true)); save(u, "06_flash_waiting", 400); }
  { Ui u; u.begin(0, false); u.s.mode = M_FLASH; Reading r = light(6.0f, true); r.flashSeq = 1;
    r.flashAmbient = r.lux; r.flashLuxSec = luxForEV(6.0f, true); u.feed(r); save(u, "07_flash_reading"); }
  { Ui u; u.begin(0, false); u.s.mode = M_COLOUR; u.s.colourCal = 1; u.feed(colour(3150, 0, 0)); save(u, "08_colour_tungsten"); }
  { Ui u; u.begin(0, false); u.s.mode = M_COLOUR; u.s.colourCal = 1; u.feed(colour(4100, 0.12f, 100)); save(u, "09_colour_fluorescent"); }
  { Ui u; u.begin(0, false); u.s.mode = M_ZONE; u.feed(light(9.0f, false)); save(u, "10_zone_start"); }
  { Ui u; u.begin(0, false); u.s.mode = M_ZONE; u.feed(light(9.0f, false)); u.addZone(6.3f); u.addZone(9.1f); u.addZone(12.0f);
    save(u, "11_zone_readings"); }
  { Ui u; u.begin(0, false); u.openChooser(0); save(u, "12_modes"); }
  { Ui u; u.begin(0, false); u.openMenu(0, false); save(u, "13_settings"); }
  { Ui u; u.begin(0, false); u.s.film = 1; u.openMenu(3, true); save(u, "14_settings_film"); }
  { Ui u; u.begin(0, false); u.openMenu(8, false); u.feed(light(11.0f, false)); u.openCal(false); save(u, "15_calibrate_light"); }
  { Ui u; u.begin(0, false); u.openCal(true); save(u, "16_calibrate_colour"); }
  { Ui u; u.begin(0, true); save(u, "17_start", 800); }
  return 0;
}
