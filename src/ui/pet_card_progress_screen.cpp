#include "pet_card_progress_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"
#include "main_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

PetCardProgressScreen petCardProgressScreen;

static constexpr int EVO_PROGRESS_BTN_X = 133;
static constexpr int EVO_PROGRESS_BTN_Y = 276;
static constexpr int EVO_PROGRESS_BTN_W = 200;
static constexpr int EVO_PROGRESS_BTN_H = 22;

/** Button that opens the evolve-or-keep dialog from the progress page. Ignores the main-screen "keep form" dismissal. */
static bool evolveButtonVisible() {
  return !pet.isEgg() && !pet.evolving() && pet.canEvolveNow();
}

void PetCardProgressScreen::handlePageTap(int16_t x, int16_t y) {
  if (!evolveButtonVisible()) return;
  if (x < EVO_PROGRESS_BTN_X || x > EVO_PROGRESS_BTN_X + EVO_PROGRESS_BTN_W) return;
  if (y < EVO_PROGRESS_BTN_Y || y > EVO_PROGRESS_BTN_Y + EVO_PROGRESS_BTN_H) return;
  // Opens the same evolve-or-keep dialog as the main screen, then returns to it.
  choiceKind = 1;
  choiceUntil = millis() + 12000;
  closePetCardPages();
}

void PetCardProgressScreen::renderPage() {
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_PROGRESS)) * 9, 44);
  gfx->print(T(S_PROGRESS));

  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  gfx->setTextSize(5);
  gfx->setCursor(PET_CARD_CX - strlen(lv) * 15, 86);
  gfx->print(lv);

  GrowthRate group = pet.growthRate();
  long long levelBase = getTotalExp(group, pet.level());
  long long levelNeed = pet.level() >= 100 ? 1 : getTotalExp(group, pet.level() + 1) - levelBase;
  long long into = pet.xp - levelBase;
  if (levelNeed <= 0) { levelNeed = 1; into = 1; }
  int bx = 93, bw = 280, by = 158, bh = 22;
  gfx->fillRoundRect(bx, by, bw, bh, 6, UI_TRACK);
  int fw = (int)((bw - 4) * into / levelNeed);
  if (fw > 0) gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 5, UI_BAR_OK);
  char nx[26];
  snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), (unsigned long)into, (unsigned long)levelNeed);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(nx) * 6, by + 32);
  gfx->print(nx);

  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_EVO_LABEL)) * 6, 230);
  gfx->print(T(S_EVO_LABEL));
  char evoBuf[28];
  const char *evo;
  uint16_t evoCol = UI_INK;
  if (!pet.hasEvolutionPath(pet.speciesId)) {
    evo = T(S_FINAL_FORM);
  } else {
    int needed = pet.evolutionRequiredLevel();
    if (pet.evolutionUnlocked()) {
      if (pet.canEvolveNow()) { evo = T(S_EVO_READY); evoCol = UI_BAR_OK; }
      else { evo = T(S_EVO_BLOCKED); evoCol = UI_BAR_BAD; }
    } else if (pet.evolutionLevelReached()) {
      evo = T(pet.evolutionNeedsMoreBond() ? S_EVO_NEED_BOND : S_EVO_BLOCKED);
      evoCol = UI_BAR_BAD;
    } else {
      snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), needed - pet.level());
      evo = evoBuf;
    }
  }
  gfx->setTextColor(evoCol);
  gfx->setCursor(PET_CARD_CX - strlen(evo) * 6, 256);
  gfx->print(evo);

  if (evolveButtonVisible()) {
    gfx->fillRoundRect(EVO_PROGRESS_BTN_X, EVO_PROGRESS_BTN_Y, EVO_PROGRESS_BTN_W, EVO_PROGRESS_BTN_H, 9, UI_BAR_OK);
    gfx->setTextColor(UI_WHITE);
    gfx->setTextSize(2);
    const char *btn = T(S_EVO_TAP);
    gfx->setCursor(PET_CARD_CX - (int)strlen(btn) * 6, EVO_PROGRESS_BTN_Y + 4);
    gfx->print(btn);
  }
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : UI_INK);
  gfx->setCursor(PET_CARD_CX - strlen(ms) * 6, 312);
  gfx->print(ms);
}
