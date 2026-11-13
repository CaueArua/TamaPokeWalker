#include "pet_card_stats_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"
#include "strength_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool cardOpen;
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);
extern void startBattle();
extern StrengthTrainingScreen strengthTrainingScreen;

PetCardStatsScreen petCardStatsScreen;

void PetCardStatsScreen::render() {
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_BATTLE)) * 9, 48);
  gfx->print(T(S_BATTLE));

  drawPetCardStat(112, T(S_STAT_ATK), pet.atkStat(), 260, UI_BAR_BAD);
  drawPetCardStat(150, T(S_STAT_DEF), pet.defStat(), 260, 0x4C98);
  drawPetCardStat(188, T(S_STAT_SPE), pet.speStat(), 260, UI_BAR_WARN);
  drawPetCardStat(226, T(S_STAT_WGT), pet.weight, 100, 0xB3C8);

  char wl[20], bs[18], bb[16];
  snprintf(wl, sizeof(wl), T(S_WL_FMT), pet.battleWins, pet.battleLosses);
  snprintf(bs, sizeof(bs), T(S_BSTREAK_FMT), pet.battleStreak);
  snprintf(bb, sizeof(bb), T(S_BBEST_FMT), pet.bestBattleStreak);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(74, 266);
  gfx->print(wl);
  gfx->setCursor(210, 266);
  gfx->print(bs);
  gfx->setCursor(334, 266);
  gfx->print(bb);

  gfx->fillRoundRect(96, 294, 274, 36, 11, 0x4C98);
  gfx->setTextColor(UI_BG_DAY);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_WILD_BATTLE)) * 6, 305);
  gfx->print(T(S_WILD_BATTLE));

  gfx->fillRoundRect(96, 338, 274, 36, 11, UI_BAR_BAD);
  gfx->setTextColor(UI_BG_DAY);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_TRAIN_STR)) * 6, 349);
  gfx->print(T(S_TRAIN_STR));
}

void PetCardStatsScreen::handlePageTap(int16_t x, int16_t y) {
  if (y >= 286 && y <= 336 && x >= 82 && x <= 384) {
    cardOpen = false;
    markUiDirty();
    lockTouchBrief();
    startBattle();
  } else if (y >= 330 && y <= 388 && x >= 82 && x <= 384) {
    cardOpen = false;
    markUiDirty();
    lockTouchBrief();
    strengthTrainingScreen.open();
  }
}
