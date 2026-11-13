#include "pet_card_steps_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

PetCardStepsScreen petCardStepsScreen;

static StrId stepRewardTextId(uint8_t index) {
  if (index == 0) return S_ITEM_SNACK;
  if (index == 1) return S_ITEM_ENERGY;
  return S_ITEM_TRAIN;
}

void PetCardStepsScreen::render() {
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_STEPS)) * 9, 44);
  gfx->print(T(S_STEPS));

  char today[28], total[28];
  snprintf(today, sizeof(today), T(S_STEPS_TODAY_FMT), (unsigned long)pet.stepsToday);
  snprintf(total, sizeof(total), T(S_STEPS_TOTAL_FMT), (unsigned long)pet.stepsTotal);
  gfx->fillRoundRect(42, 76, 184, 58, 14, UI_WHITE);
  gfx->drawRoundRect(42, 76, 184, 58, 14, UI_BAR_OK);
  gfx->fillRoundRect(240, 76, 184, 58, 14, UI_WHITE);
  gfx->drawRoundRect(240, 76, 184, 58, 14, UI_BAR_WARN);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(42 + (184 - (int)strlen(today) * 12) / 2, 99);
  gfx->print(today);
  gfx->setCursor(240 + (184 - (int)strlen(total) * 12) / 2, 99);
  gfx->print(total);

  char rank[24], shiny[24], catchBuf[24];
  snprintf(rank, sizeof(rank), T(S_TRAIL_RANK_FMT), pet.stepTrailRank());
  uint16_t units = pet.stepShinyChancePer4096();
  uint16_t denominator = units ? (uint16_t)((4096U + units / 2U) / units) : 0;
  snprintf(shiny, sizeof(shiny), T(S_SHINY_LUCK_FMT), denominator);
  snprintf(catchBuf, sizeof(catchBuf), T(S_CATCH_BONUS_FMT), pet.stepCatchBonus());
  gfx->setTextColor(UI_BAR_WARN);
  gfx->setCursor(PET_CARD_CX - (int)strlen(rank) * 6, 158);
  gfx->print(rank);
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(1);
  gfx->setCursor(76, 184);
  gfx->print(shiny);
  gfx->setCursor(300, 184);
  gfx->print(catchBuf);

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_DAILY)) * 6, 208);
  gfx->print(T(S_DAILY));
  for (uint8_t i = 0; i < STEP_DAILY_GOAL_COUNT; i++) {
    uint32_t goal = pet.stepGoal(i);
    uint32_t progress = pet.stepsToday < goal ? pet.stepsToday : goal;
    bool done = pet.stepGoalComplete(i);
    int y = 224 + i * 48;
    uint16_t col = i == 0 ? UI_BAR_OK : (i == 1 ? UI_BAR_WARN : UI_BAR_BAD);
    gfx->fillRoundRect(48, y, 370, 38, 10, done ? col : UI_WHITE);
    gfx->drawRoundRect(48, y, 370, 38, 10, col);
    gfx->setTextColor(done ? UI_BG_DAY : UI_INK);
    gfx->setTextSize(1);
    char goalText[18], prog[18];
    snprintf(goalText, sizeof(goalText), T(S_STEP_GOAL_FMT), (unsigned long)goal);
    snprintf(prog, sizeof(prog), "%lu/%lu", (unsigned long)progress, (unsigned long)goal);
    gfx->setCursor(66, y + 9);
    gfx->print(goalText);
    gfx->setCursor(232, y + 9);
    gfx->print(T(stepRewardTextId(i)));
    gfx->setCursor(354, y + 9);
    gfx->print(done ? T(S_DONE) : prog);
  }
}
