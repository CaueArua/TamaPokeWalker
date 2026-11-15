#include "render_utils.h"

#include <Arduino_GFX_Library.h>

#include "../../species.h"

extern Arduino_Canvas *gfx;
extern PmdMon pmd;

static constexpr int GALLERY_CELL_SIZE = 74;
static constexpr uint16_t SPRITE_INK = 0x18C4;

uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

void drawThumb(const uint8_t *b, int x, int y, int s, bool sil) {
  uint8_t w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3;
  const uint8_t *d = pal + n * 2;
  int ox = x + (GALLERY_CELL_SIZE - w * s) / 2;
  int oy = y + (GALLERY_CELL_SIZE - h * s) / 2;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      uint8_t idx = d[r * w + c];
      if (idx == 0xFF) continue;
      uint16_t col = sil ? SPRITE_INK : (uint16_t)(pal[idx * 2] | (pal[idx * 2 + 1] << 8));
      gfx->fillRect(ox + c * s, oy + r * s, s, s, col);
    }
  }
}

uint32_t pmdActTotalMs(const PmdAct &action) {
  uint32_t duration = 0;
  for (uint8_t i = 0; i < action.frames; i++) duration += action.ms[i];
  return duration ? duration : 100;
}

uint8_t pmdFrameAt(const PmdAct &action, uint32_t elapsedMs, bool loop) {
  uint32_t total = pmdActTotalMs(action);
  if (!loop && elapsedMs >= total) return action.frames - 1;
  elapsedMs %= total;
  uint8_t frame = 0;
  while (elapsedMs >= action.ms[frame]) {
    elapsedMs -= action.ms[frame];
    frame = (frame + 1) % action.frames;
  }
  return frame;
}

void drawPmdActM(PmdMon &mon, uint8_t action, int cx, int groundY,
                 uint32_t elapsedMs, bool loop, bool silhouette, uint8_t maxScale,
                 bool uniformScale) {
  const PmdAct &animation = mon.acts[action];
  if (!animation.frames) return;
  // Per-species scale would cancel out differences in canvas size, so pets of different sizes look equal.
  uint8_t scale = uniformScale ? maxScale : (mon.acts[PMD_IDLE].h ? 170 / mon.acts[PMD_IDLE].h : 5);
  if (scale < 2) scale = 2;
  if (scale > maxScale) scale = maxScale;
  while (scale > 2 && animation.h * scale > 250) scale--;
  uint8_t frame = pmdFrameAt(animation, elapsedMs, loop);
  const uint8_t *pixels = animation.data + (uint32_t)frame * animation.w * animation.h;
  int x0 = cx - animation.w * scale / 2;
  int y0 = groundY - (animation.base ? animation.base : animation.h) * scale;
  for (int r = 0; r < animation.h; r++) {
    const uint8_t *row = pixels + r * animation.w;
    for (int c = 0; c < animation.w; c++) {
      uint8_t idx = row[c];
      if (idx == 0xFF) continue;
      gfx->fillRect(x0 + c * scale, y0 + r * scale, scale, scale,
                    silhouette ? SPRITE_INK : mon.pal[idx]);
    }
  }
}

void drawPmdAct(uint8_t action, int cx, int groundY, uint32_t elapsedMs,
                bool loop, bool silhouette, uint8_t maxScale, bool uniformScale) {
  drawPmdActM(pmd, action, cx, groundY, elapsedMs, loop, silhouette, maxScale, uniformScale);
}

void drawMap(const char *const *map, int size, int x, int y, int scale, bool silhouette) {
  for (int r = 0; r < size; r++) {
    for (int c = 0; c < size; c++) {
      char pixel = map[r][c];
      if (pixel == '.') continue;
      gfx->fillRect(x + c * scale, y + r * scale, scale, scale,
                    silhouette ? SPRITE_INK : spriteColor(pixel));
    }
  }
}
