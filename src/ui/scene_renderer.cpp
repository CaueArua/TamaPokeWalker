#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "../../dayphase.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../pin_config.h"
#include "../../species.h"
#include "../../time_utils.h"
#include "power_manager.h"
#include "render_utils.h"
#include "scene_renderer.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

static int8_t watermarkDrawnSlot = -1;
const uint16_t STARS[6][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };
// Shared background and watermark rendering.

bool gNight = false;  // Set by render() from local time or pet sleep state.
/**
 * Returns the foreground ink color for the current day/night scene.
 *
 * @return The active UI ink color.
 */
uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// Background-scene rendering based on biome and real-time clock.

#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define HORIZON 232  // Boundary between the sky and ground.

/**
 * Selects the watermark corner, rotating every 20 seconds.
 *
 * @return The current corner index.
 */
static uint8_t watermarkSlot() {
  return (uint8_t)((millis() / 20000UL) & 3U);
}

/**
 * Indicates whether the watermark has moved to a new corner and needs redrawing.
 *
 * @return True when the current frame should be refreshed.
 */
bool watermarkNeedsRedraw() {
  return !screenOff && watermarkDrawnSlot != (int8_t)watermarkSlot();
}

/**
 * Blends a watermark tint into a pixel while preserving contrast.
 *
 * @param base Original RGB565 pixel color.
 * @return The tinted RGB565 color.
 */
static uint16_t watermarkPixel(uint16_t base) {
  int r = ((base >> 11) & 31) * 255 / 31;
  int g = ((base >> 5) & 63) * 255 / 63;
  int b = (base & 31) * 255 / 31;
  int luminance = (r * 30 + g * 59 + b * 11) / 100;
  return lerp565(base, luminance > 145 ? 0x0000 : 0xFFFF, 1, 4);
}

/**
 * Draws the small watermark glyphs into the current framebuffer slot.
 *
 * @param slot Watermark corner index.
 */
static void drawWatermark(uint8_t slot) {
  static const uint8_t GLYPHS[3][7] = {
    { 0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E },  // @
    { 0x1F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E },  // S
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F },  // E
  };
  static const int16_t POS[4][2] = {
    { 74, 74 }, { 374, 74 }, { 74, 384 }, { 374, 384 },
  };
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int16_t ox = POS[slot][0];
  int16_t oy = POS[slot][1];
  for (uint8_t glyph = 0; glyph < 3; glyph++) {
    for (uint8_t row = 0; row < 7; row++) {
      uint8_t bits = GLYPHS[glyph][row];
      for (uint8_t col = 0; col < 5; col++) {
        if (!(bits & (1U << (4 - col)))) continue;
        int16_t x = ox + glyph * 6 + col;
        int16_t y = oy + row;
        if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) continue;
        uint32_t idx = (uint32_t)y * LCD_WIDTH + x;
        fb[idx] = watermarkPixel(fb[idx]);
      }
    }
  }
}

/**
 * Stamps the current frame with a watermark and submits it to the display.
 */
void flushStampedFrame() {
  uint8_t slot = watermarkSlot();
  drawWatermark(slot);
  watermarkDrawnSlot = (int8_t)slot;
  gfx->flush();
}

/**
 * Returns the pet's current local-scene hour derived from its synchronized clock.
 *
 * @return Hour of day in the range 0-23.
 */
int sceneHour() {
  return sceneHourFromEpoch(pet.lastSeenEpoch);
}

/**
 * Maps the current scene hour to a day-phase category.
 *
 * @return The phase identifier consumed by encounter and UI logic.
 */
uint8_t currentDayPhase() {
  return dayPhaseFromHour(sceneHour());
}

/**
 * Maps a day-phase identifier to its localized text resource.
 *
 * @param phase Day-phase identifier.
 * @return The matching localization string identifier.
 */
StrId dayPhaseTextId(uint8_t phase) {
  switch (phase) {
    case 0: return S_MORNING;
    case 2: return S_EVENING;
    case 3: return S_NIGHT;
    default: return S_DAY;
  }
}

// Daytime ground colors for each biome; night scenes blend them toward blue.
const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 meadow
  C565(0xdc, 0xca, 0x94),  // 1 beach
  C565(0x4f, 0x8a, 0x55),  // 2 forest
  C565(0x8a, 0x55, 0x44),  // 3 volcano
  C565(0xa8, 0x90, 0x6a),  // 4 mountain
  C565(0xe6, 0xee, 0xf5),  // 5 snow
};
/**
 * Draws the shared minigame background using the current hour and pet biome.
 */
void drawGameScene() {
  int hh = sceneHour();
  bool night = hh < 6 || hh >= 20;
  uint16_t top, bot;
  if (night)       { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (hh < 8) { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }
  else if (hh < 18){ top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }
  else             { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }
  int hor = 376;
  for (int y = 0; y < hor; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, hor));
  if (night)
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  uint8_t bio = pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  gfx->fillRect(0, hor, 466, 466 - hor, soil);
}

