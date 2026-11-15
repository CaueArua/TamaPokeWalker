#include "pet_card_summary_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../audio.h"
#include "render_utils.h"
#include "touch_controller.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon pmd;
extern const char *dexName(int16_t dex);

PetCardSummaryScreen petCardSummaryScreen;

static constexpr int SUM_SPRITE_CX = 122;
static constexpr int SUM_SPRITE_GROUND_Y = 236;
static constexpr int SUM_STAR_X = 200, SUM_STAR_Y = 110;
static constexpr int SUM_BTN_X = 246, SUM_BTN_W = 164, SUM_BTN_H = 52;
static constexpr int SUM_BTN_COUNT = 4;
static const int SUM_BTN_Y[SUM_BTN_COUNT] = {110, 178, 246, 314};
static const StrId SUM_BTN_LABEL[SUM_BTN_COUNT] = {S_SUM_INFOS, S_SUM_STATUS, S_SUM_MEDALS, S_PROGRESS};
static const uint8_t SUM_BTN_PAGE[SUM_BTN_COUNT] = {
  PET_CARD_PAGE_INFOS, PET_CARD_PAGE_STATS, PET_CARD_PAGE_MEDALS, PET_CARD_PAGE_PROGRESS,
};

/** Draws a gold asterisk marking a shiny pet. */
static void drawGoldAsterisk(int cx, int cy) {
  const int r = 9;
  for (int d = 0; d <= 1; d++) {
    gfx->drawLine(cx - r + d, cy, cx + r + d, cy, UI_GOLD);
    gfx->drawLine(cx + d, cy - r, cx + d, cy + r, UI_GOLD);
    gfx->drawLine(cx - r + d, cy - r, cx + r + d, cy + r, UI_GOLD);
    gfx->drawLine(cx - r + d, cy + r, cx + r + d, cy - r, UI_GOLD);
  }
  gfx->fillCircle(cx, cy, 2, UI_GOLD);
}

void PetCardSummaryScreen::renderPage() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  if (pmd.loaded) drawPmdAct(PMD_IDLE, SUM_SPRITE_CX, SUM_SPRITE_GROUND_Y, millis(), true, false, 3);
  if (pet.shiny) drawGoldAsterisk(SUM_STAR_X, SUM_STAR_Y);

  char lvl[12], head[30];
  snprintf(lvl, sizeof(lvl), T(S_LVL_FMT), pet.level());
  snprintf(head, sizeof(head), "%s %s", nm, lvl);
  gfx->setTextSize(2);
  gfx->setTextColor(d.accent);
  gfx->setCursor(SUM_SPRITE_CX - (int)strlen(head) * 6, 296);
  gfx->print(head);
  if (pet.nick[0]) {
    const char *speciesName = dexName(pet.speciesId);
    gfx->setTextColor(UI_INK);
    gfx->setCursor(SUM_SPRITE_CX - (int)strlen(speciesName) * 6, 322);
    gfx->print(speciesName);
  }

  for (int i = 0; i < SUM_BTN_COUNT; i++) {
    gfx->fillRoundRect(SUM_BTN_X, SUM_BTN_Y[i], SUM_BTN_W, SUM_BTN_H, 11, UI_WHITE);
    gfx->drawRoundRect(SUM_BTN_X, SUM_BTN_Y[i], SUM_BTN_W, SUM_BTN_H, 11, UI_INK);
    const char *label = T(SUM_BTN_LABEL[i]);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(SUM_BTN_X + SUM_BTN_W / 2 - (int)strlen(label) * 6, SUM_BTN_Y[i] + 18);
    gfx->print(label);
  }
}

void PetCardSummaryScreen::handlePageTap(int16_t x, int16_t y) {
  if (x < SUM_BTN_X || x > SUM_BTN_X + SUM_BTN_W) return;
  for (int i = 0; i < SUM_BTN_COUNT; i++) {
    if (y >= SUM_BTN_Y[i] && y < SUM_BTN_Y[i] + SUM_BTN_H) {
      lockTouchBrief();
      sfxPlay(SFX_TAP);
      openPetCardSubpage(SUM_BTN_PAGE[i]);
      return;
    }
  }
}
