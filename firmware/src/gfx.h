// A small software canvas: the whole screen is drawn into RAM and pushed in one go, so nothing flickers and
// the same drawing code runs on the meter and on a computer (tools/host renders the screens to PNG).
#pragma once
#include <stdint.h>
#include "fonts.h"

constexpr int W = 368, H = 448;   // the AMOLED, upright

struct RGB { uint8_t r, g, b; };

namespace col {
constexpr RGB black{0, 0, 0};
constexpr RGB white{242, 240, 236};   // warm white, like paper
constexpr RGB grey{138, 136, 132};
constexpr RGB dim{70, 69, 67};
constexpr RGB faint{34, 34, 33};
constexpr RGB red{226, 6, 19};        // the red dot
constexpr RGB redDim{110, 12, 16};
}

enum Align { LEFT, CENTER, RIGHT };

struct Canvas {
  uint16_t *fb;  // W*H pixels, RGB565

  static uint16_t pack(RGB c) { return (uint16_t)(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3)); }
  static RGB unpack(uint16_t p) {
    uint8_t r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
    return RGB{(uint8_t)((r << 3) | (r >> 2)), (uint8_t)((g << 2) | (g >> 4)), (uint8_t)((b << 3) | (b >> 2))};
  }
  static RGB mix(RGB a, RGB b, int t255) {  // t = 0: a, 255: b
    return RGB{(uint8_t)(a.r + ((b.r - a.r) * t255) / 255), (uint8_t)(a.g + ((b.g - a.g) * t255) / 255),
               (uint8_t)(a.b + ((b.b - a.b) * t255) / 255)};
  }

  void clear(RGB c = col::black);
  void blend(int x, int y, RGB c, int a255);
  void fillRect(int x, int y, int w, int h, RGB c);
  void hline(int x, int y, int w, RGB c) { fillRect(x, y, w, 1, c); }
  void vline(int x, int y, int h, RGB c) { fillRect(x, y, 1, h, c); }
  void disc(float cx, float cy, float r, RGB c, int a255 = 255);            // anti-aliased
  void ring(float cx, float cy, float r, float t, RGB c, int a255 = 255);   // anti-aliased
  void triangle(float x0, float y0, float x1, float y1, float x2, float y2, RGB c, int a255 = 255);

  // Text: x is the pen position (see Align), y the baseline. Returns the width drawn.
  int text(const Font &f, int x, int y, const char *s, RGB c, Align al = LEFT, int a255 = 255, int track = 0);
  static int width(const Font &f, const char *s, int track = 0);
};
