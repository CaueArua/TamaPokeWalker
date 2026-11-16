#include "pet_card_stats_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

PetCardStatsScreen petCardStatsScreen;

extern const char *dexName(int16_t dex);

// Nature highlight colors (RGB565), local to this screen.
static constexpr uint16_t NATURE_UP_COLOR = 0xF800;   // red: stat boosted by the nature
static constexpr uint16_t NATURE_DOWN_COLOR = 0x001F; // blue: stat lowered by the nature

// Five hearts, one per 51 points of bond (255 fills all five). The last heart fills
// from left to right as the bond grows inside its range.
static void drawBondHearts(uint8_t bond) {
  static constexpr int HEART_COUNT = 5;
  static constexpr int HEART_STEP = 51;
  static constexpr int HALF = 22;
  static constexpr float SCALE = 18.0f;
  for (int h = 0; h < HEART_COUNT; h++) {
    const int cx = 118 + h * 57;
    const int cy = 352;
    int rem = bond - h * HEART_STEP;
    if (rem < 0) rem = 0;
    if (rem > HEART_STEP) rem = HEART_STEP;
    const int fillX = cx - HALF + rem * (2 * HALF + 1) / HEART_STEP;
    for (int py = cy - HALF; py <= cy + HALF; py++) {
      const float ny = (cy - py) / SCALE;
      for (int px = cx - HALF; px <= cx + HALF; px++) {
        const float nx = (px - cx) / SCALE;
        const float a = nx * nx + ny * ny - 1.0f;
        // Implicit heart curve: (x^2 + y^2 - 1)^3 - x^2 * y^3 <= 0.
        if (a * a * a - nx * nx * ny * ny * ny > 0) continue;
        gfx->drawPixel(px, py, rem > 0 && px < fillX ? UI_BAR_BAD : UI_TRACK);
      }
    }
  }
}

void PetCardStatsScreen::renderPage() {
  const char *name = pet.isEgg() ? "Egg" : ((pet.nick[0] != '\0') ? pet.nick : dexName(pet.speciesId));
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(name) * 9, 40);
  gfx->print(name);

  char head[40];
  snprintf(head, sizeof(head), "Lv.%u  Nature: %s", pet.level(), natureName(pet.nature));
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(head) * 6, 84);
  gfx->print(head);

  static const char *const STAT_LABELS[STAT_COUNT] = { "HP", "ATK", "DEF", "SP.A", "SP.D", "SPE" };
  gfx->setTextSize(1);
  gfx->setCursor(60, 116);
  gfx->print("STAT");
  gfx->setCursor(150, 116);
  gfx->print("EV /252");
  gfx->setCursor(340, 116);
  gfx->print("TOTAL");

  for (uint8_t i = 0; i < STAT_COUNT; i++) {
    int y = 136 + i * 30;
    int8_t mod = natureStatMod(pet.nature, i);
    char label[8];
    snprintf(label, sizeof(label), "%c%s", mod > 0 ? '+' : (mod < 0 ? '-' : ' '), STAT_LABELS[i]);
    gfx->setTextSize(2);
    gfx->setTextColor(mod > 0 ? NATURE_UP_COLOR : (mod < 0 ? NATURE_DOWN_COLOR : UI_INK));
    gfx->setCursor(60, y);
    gfx->print(label);

    static constexpr int EV_BAR_X = 150;
    static constexpr int EV_BAR_W = 170;
    static constexpr int EV_BAR_H = 14;
    static constexpr int EV_BAR_RADIUS = 7;
    gfx->fillRoundRect(EV_BAR_X, y + 2, EV_BAR_W, EV_BAR_H, EV_BAR_RADIUS, UI_TRACK);
    int fill = pet.evs[i] * EV_BAR_W / EV_STAT_MAX;
    if (fill > 0) {
      gfx->fillRoundRect(EV_BAR_X, y + 2, fill, EV_BAR_H, EV_BAR_RADIUS, 0x4C98);
      if (fill < EV_BAR_W && fill > EV_BAR_RADIUS) {
        gfx->fillRect(EV_BAR_X + EV_BAR_RADIUS, y + 2, fill - EV_BAR_RADIUS,
                      EV_BAR_H, 0x4C98);
      }
    }

    char num[8];
    gfx->setTextColor(UI_INK);
    snprintf(num, sizeof(num), "%u", pet.statValue(i));
    gfx->setCursor(340, y);
    gfx->print(num);
  }

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen("Afeicao") * 6, 310);
  gfx->print("Afeicao");
  drawBondHearts(pet.bond);
}

void PetCardStatsScreen::handlePageTap(int16_t, int16_t) {}
