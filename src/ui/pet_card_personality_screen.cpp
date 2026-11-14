#include "pet_card_personality_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

PetCardPersonalityScreen petCardPersonalityScreen;

static StrId personalityNameId(PetPersonality p) {
  switch (p) {
    case PERS_PLAYFUL: return S_PERS_PLAYFUL;
    case PERS_BRAVE: return S_PERS_BRAVE;
    case PERS_CALM: return S_PERS_CALM;
    case PERS_LAZY: return S_PERS_LAZY;
    default: return S_PERS_BALANCED;
  }
}

static StrId personalityHintId(PetPersonality p) {
  switch (p) {
    case PERS_PLAYFUL: return S_PERS_PLAYFUL_HINT;
    case PERS_BRAVE: return S_PERS_BRAVE_HINT;
    case PERS_CALM: return S_PERS_CALM_HINT;
    case PERS_LAZY: return S_PERS_LAZY_HINT;
    default: return S_PERS_BALANCED_HINT;
  }
}

static uint16_t personalityColor(PetPersonality p) {
  switch (p) {
    case PERS_PLAYFUL: return UI_BAR_WARN;
    case PERS_BRAVE: return UI_BAR_BAD;
    case PERS_CALM: return 0x4C98;
    case PERS_LAZY: return 0xB3C8;
    default: return UI_BAR_OK;
  }
}

static void drawPersonalityRecord(int x, int y, const char *label, uint16_t val, uint16_t color) {
  gfx->fillRoundRect(x, y, 118, 34, 8, UI_WHITE);
  gfx->drawRoundRect(x, y, 118, 34, 8, color);
  gfx->setTextColor(color);
  gfx->setTextSize(1);
  gfx->setCursor(x + 10, y + 6);
  gfx->print(label);
  char num[8];
  snprintf(num, sizeof(num), "%u", val);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(x + 118 - 12 - (int)strlen(num) * 12, y + 14);
  gfx->print(num);
}

void PetCardPersonalityScreen::renderPage() {
  PetPersonality pers = pet.personality();
  const char *title = T(S_PERSONALITY);
  const char *name = T(personalityNameId(pers));
  const char *hint = T(personalityHintId(pers));
  uint16_t col = personalityColor(pers);

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(title) * 9, 44);
  gfx->print(title);

  gfx->fillRoundRect(62, 86, 342, 70, 16, col);
  gfx->setTextColor(UI_BG_DAY);
  int nts = (strlen(name) <= 10) ? 3 : 2;
  gfx->setTextSize(nts);
  gfx->setCursor(PET_CARD_CX - strlen(name) * (nts == 3 ? 9 : 6), nts == 3 ? 104 : 111);
  gfx->print(name);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(hint) * 6, 136);
  gfx->print(hint);

  drawPetCardStat(182, T(S_VIN), pet.bond, 100, C565(0xd4, 0x52, 0x7e));
  drawPetCardStat(220, T(S_BAR_JOY), pet.joy, 100, UI_BAR_WARN);

  char age[20];
  snprintf(age, sizeof(age), T(S_AGE_DAYS_FMT), (unsigned long)(pet.ageMinutes / 1440));
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(age) * 6, 258);
  gfx->print(age);

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_RECORDS)) * 6, 270);
  gfx->print(T(S_RECORDS));
  drawPersonalityRecord(52, 294, T(S_GAME_BALL), pet.gameHi, UI_BAR_OK);
  drawPersonalityRecord(178, 294, T(S_GAME_CATCH), pet.catchHi, UI_BAR_WARN);
  drawPersonalityRecord(304, 294, T(S_GAME_MEMO), pet.memoHi, 0x4C98);
  drawPersonalityRecord(52, 334, T(S_GAME_CLEAN), pet.cleanHi, UI_BAR_OK);
  drawPersonalityRecord(178, 334, T(S_GAME_TYPE), pet.typeHi, 0xF3B7);
  drawPersonalityRecord(304, 334, T(S_BATTLE), pet.bestBattleStreak, UI_BAR_BAD);
}
