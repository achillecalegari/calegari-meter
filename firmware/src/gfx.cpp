#include "gfx.h"
#include <math.h>

static inline int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

void Canvas::clear(RGB c) {
  uint16_t p = pack(c);
  for (int i = 0; i < W * H; i++) fb[i] = p;
}

void Canvas::blend(int x, int y, RGB c, int a) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || a <= 0) return;
  uint16_t *p = &fb[y * W + x];
  *p = a >= 255 ? pack(c) : pack(mix(unpack(*p), c, a));
}

void Canvas::fillRect(int x, int y, int w, int h, RGB c) {
  int x0 = clampi(x, 0, W), x1 = clampi(x + w, 0, W), y0 = clampi(y, 0, H), y1 = clampi(y + h, 0, H);
  uint16_t p = pack(c);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) fb[yy * W + xx] = p;
}

void Canvas::disc(float cx, float cy, float r, RGB c, int a) {
  for (int y = (int)floorf(cy - r - 1); y <= (int)ceilf(cy + r + 1); y++)
    for (int x = (int)floorf(cx - r - 1); x <= (int)ceilf(cx + r + 1); x++) {
      float d = sqrtf((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy));
      float cov = r + 0.5f - d;
      if (cov > 0) blend(x, y, c, (int)(a * (cov > 1 ? 1 : cov)));
    }
}

void Canvas::ring(float cx, float cy, float r, float t, RGB c, int a) {
  for (int y = (int)floorf(cy - r - 1); y <= (int)ceilf(cy + r + 1); y++)
    for (int x = (int)floorf(cx - r - 1); x <= (int)ceilf(cx + r + 1); x++) {
      float d = sqrtf((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy));
      float cov = fminf(r + 0.5f - d, d - (r - t) + 0.5f);
      if (cov > 0) blend(x, y, c, (int)(a * (cov > 1 ? 1 : cov)));
    }
}

void Canvas::triangle(float x0, float y0, float x1, float y1, float x2, float y2, RGB c, int a) {
  // 4x4 supersampled coverage: small shapes only (indices, arrows)
  int xa = (int)floorf(fminf(x0, fminf(x1, x2))), xb = (int)ceilf(fmaxf(x0, fmaxf(x1, x2)));
  int ya = (int)floorf(fminf(y0, fminf(y1, y2))), yb = (int)ceilf(fmaxf(y0, fmaxf(y1, y2)));
  auto edge = [](float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
  };
  float area = edge(x0, y0, x1, y1, x2, y2);
  for (int y = ya; y <= yb; y++)
    for (int x = xa; x <= xb; x++) {
      int n = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          float px = x + (sx + 0.5f) / 4, py = y + (sy + 0.5f) / 4;
          float w0 = edge(x1, y1, x2, y2, px, py), w1 = edge(x2, y2, x0, y0, px, py), w2 = edge(x0, y0, x1, y1, px, py);
          if (area > 0 ? (w0 >= 0 && w1 >= 0 && w2 >= 0) : (w0 <= 0 && w1 <= 0 && w2 <= 0)) n++;
        }
      if (n) blend(x, y, c, a * n / 16);
    }
}

int Canvas::width(const Font &f, const char *s, int track) {
  int w = 0, n = 0;
  for (const uint8_t *p = (const uint8_t *)s; *p; p++, n++) {
    if (*p < f.first || *p > f.last) continue;
    w += f.glyphs[*p - f.first].adv + track;
  }
  return n ? w - track : 0;
}

int Canvas::text(const Font &f, int x, int y, const char *s, RGB c, Align al, int a, int track) {
  int w = width(f, s, track);
  if (al == CENTER) x -= w / 2;
  else if (al == RIGHT) x -= w;
  for (const uint8_t *p = (const uint8_t *)s; *p; p++) {
    if (*p < f.first || *p > f.last) continue;
    const Glyph &g = f.glyphs[*p - f.first];
    const uint8_t *bits = f.bits + g.off;
    int stride = (g.w + 1) / 2;
    for (int yy = 0; yy < g.h; yy++)
      for (int xx = 0; xx < g.w; xx++) {
        uint8_t b = bits[yy * stride + xx / 2];
        int v = (xx & 1) ? (b & 15) : (b >> 4);
        if (v) blend(x + g.dx + xx, y + g.dy + yy, c, a * v / 15);
      }
    x += g.adv + track;
  }
  return w;
}
