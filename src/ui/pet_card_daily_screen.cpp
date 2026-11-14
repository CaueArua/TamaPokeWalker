#include "pet_card_daily_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern uint8_t currentDayPhase();
extern StrId dayPhaseTextId(uint8_t phase);

PetCardDailyScreen petCardDailyScreen;

static StrId dailyGoalLabelId(uint8_t goalType) {
  switch (goalType) {
    case DAILY_GOAL_CARE: return S_GOAL_CARE;
    case DAILY_GOAL_PLAY: return S_GOAL_PLAY;
    case DAILY_GOAL_BATTLE: return S_GOAL_BATTLE;
    case DAILY_GOAL_CATCH: return S_GOAL_CATCH;
    case DAILY_GOAL_MEMO: return S_GOAL_MEMO;
    default: return S_GOAL_CARE;
  }
}

static uint16_t dailyGoalColor(uint8_t goalType) {
  switch (goalType) {
    case DAILY_GOAL_CARE: return C565(0xd4, 0x52, 0x7e);
    case DAILY_GOAL_PLAY: return UI_BAR_WARN;
    case DAILY_GOAL_BATTLE: return UI_BAR_BAD;
    case DAILY_GOAL_CATCH: return UI_BAR_OK;
    case DAILY_GOAL_MEMO: return 0x4C98;
    default: return UI_INK;
  }
}

static void drawDailyGoalRow(int y, uint8_t idx) {
  uint8_t type = pet.dailyGoalType[idx];
  uint8_t target = pet.dailyGoalTarget(type);
  uint8_t progress = pet.dailyGoalProgress[idx] > target ? target : pet.dailyGoalProgress[idx];
  bool done = pet.dailyGoalComplete(idx);
  uint16_t col = dailyGoalColor(type);
  gfx->fillRoundRect(58, y, 350, 52, 12, done ? col : UI_WHITE);
  gfx->drawRoundRect(58, y, 350, 52, 12, col);
  gfx->setTextSize(2);
  gfx->setTextColor(done ? UI_BG_DAY : UI_INK);
  gfx->setCursor(82, y + 18);
  gfx->print(T(dailyGoalLabelId(type)));

  char prog[12];
  snprintf(prog, sizeof(prog), "%u/%u", progress, target);
  gfx->setCursor(286, y + 18);
  gfx->print(done ? T(S_DONE) : prog);
  if (done) {
    gfx->fillCircle(374, y + 26, 12, UI_BG_DAY);
    gfx->setTextColor(col);
    gfx->setCursor(368, y + 18);
    gfx->print("v");
  }
}

void PetCardDailyScreen::renderPage() {
  pet.ensureDailyGoals();
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_DAILY)) * 9, 44);
  gfx->print(T(S_DAILY));
  const char *phase = T(dayPhaseTextId(currentDayPhase()));
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(phase) * 6, 74);
  gfx->print(phase);

  uint8_t done = 0;
  for (uint8_t i = 0; i < DAILY_GOAL_COUNT; i++) {
    if (pet.dailyGoalComplete(i)) done++;
    drawDailyGoalRow(104 + i * 70, i);
  }

  char bonus[24];
  snprintf(bonus, sizeof(bonus), "%s %u/%u", T(S_REWARD), done, DAILY_GOAL_COUNT);
  gfx->setTextColor(done == DAILY_GOAL_COUNT ? UI_BAR_OK : UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(bonus) * 6, 324);
  gfx->print(bonus);
}
