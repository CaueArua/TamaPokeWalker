#include "pet_card_expedition_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../i18n.h"
#include "../../audio.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool cardOpen;
extern bool cardDirty;
extern bool expeditionTrainChoiceOpen;
extern uint32_t expeditionNowEpoch();
extern StrId expeditionItemText(ExpeditionItem item);
extern StrId expeditionSiteText(ExpeditionSite site);
extern uint16_t expeditionSiteColor(ExpeditionSite site);
extern uint16_t expeditionItemColor(ExpeditionItem item);
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);

PetCardExpeditionScreen petCardExpeditionScreen;

static void drawExpeditionItem(int x, int y, ExpeditionItem item) {
  const int w = 172, h = 54;
  uint8_t count = pet.itemCounts[item];
  uint16_t col = expeditionItemColor(item);
  gfx->fillRoundRect(x, y, w, h, 9, count ? UI_WHITE : C565(0xe4, 0xe8, 0xee));
  gfx->drawRoundRect(x, y, w, h, 9, count ? col : UI_TRACK);
  gfx->fillCircle(x + 22, y + 27, 12, count ? col : UI_TRACK);
  if (item == EXP_ITEM_ENERGY) gfx->fillRect(x + 20, y + 17, 5, 20, UI_WHITE);
  else if (item == EXP_ITEM_CARE) gfx->fillCircle(x + 22, y + 22, 4, UI_WHITE);
  else if (item == EXP_ITEM_TRAIN) gfx->fillRect(x + 16, y + 25, 12, 4, UI_WHITE);

  const char *label = T(expeditionItemText(item));
  gfx->setTextSize(1);
  gfx->setTextColor(count ? UI_INK : UI_TRACK);
  gfx->setCursor(x + 42, y + 16);
  gfx->print(label);
  char amount[6];
  snprintf(amount, sizeof(amount), "x%u", count);
  gfx->setTextSize(2);
  gfx->setCursor(x + 136, y + 29);
  gfx->print(amount);
}

static void renderExpeditionTrainChoice() {
  gfx->fillRoundRect(58, 118, 350, 190, 14, UI_WHITE);
  gfx->drawRoundRect(58, 118, 350, 190, 14, UI_INK);
  const char *title = T(S_ITEM_TRAIN);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(title) * 6, 136);
  gfx->print(title);

  const StrId labels[3] = { S_TRAIN_ATK, S_TRAIN_DEF, S_TRAIN_SPE };
  const uint8_t values[3] = { pet.trAtk, pet.trDef, pet.trSpe };
  const uint16_t cols[3] = { UI_BAR_BAD, 0x4C98, UI_BAR_WARN };
  for (uint8_t i = 0; i < 3; i++) {
    int x = 74 + i * 108;
    bool usable = values[i] < 100;
    gfx->fillRoundRect(x, 172, 102, 66, 9, usable ? cols[i] : UI_TRACK);
    gfx->setTextColor(UI_BG_DAY);
    gfx->setTextSize(2);
    const char *label = T(labels[i]);
    gfx->setCursor(x + (102 - (int)strlen(label) * 12) / 2, 184);
    gfx->print(label);
    if (usable) {
      gfx->setTextSize(1);
      gfx->setCursor(x + 37, 210);
      gfx->print("+2");
    } else {
      const char *maxed = T(S_ITEM_MAXED);
      gfx->setTextSize(1);
      gfx->setCursor(x + (102 - (int)strlen(maxed) * 6) / 2, 210);
      gfx->print(maxed);
    }
  }
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_BACK)) * 6, 268);
  gfx->print(T(S_BACK));
}

void PetCardExpeditionScreen::render() {
  uint32_t nowEpoch = expeditionNowEpoch();
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_EXPEDITION)) * 9, 42);
  gfx->print(T(S_EXPEDITION));

  if (pet.expeditionReady(nowEpoch)) {
    char found[34];
    snprintf(found, sizeof(found), T(S_FOUND_ITEM_FMT), T(expeditionItemText((ExpeditionItem)pet.expeditionRewardItem)));
    gfx->setTextColor(UI_BAR_OK);
    gfx->setTextSize(2);
    gfx->setCursor(PET_CARD_CX - strlen(found) * 6, 78);
    gfx->print(found);
    gfx->setTextColor(expeditionSiteColor((ExpeditionSite)pet.expeditionSite));
    gfx->setTextSize(1);
    gfx->setCursor(PET_CARD_CX - (int)strlen(T(expeditionSiteText((ExpeditionSite)pet.expeditionSite))) * 3, 92);
    gfx->print(T(expeditionSiteText((ExpeditionSite)pet.expeditionSite)));
    gfx->fillRoundRect(98, 98, 270, 48, 11, UI_BAR_OK);
    gfx->setTextColor(UI_BG_DAY);
    gfx->setTextSize(2);
    gfx->setCursor(PET_CARD_CX - strlen(T(S_EXP_CLAIM)) * 6, 111);
    gfx->print(T(S_EXP_CLAIM));
  } else if (pet.expeditionActive(nowEpoch)) {
    uint32_t left = (pet.expeditionEndEpoch - nowEpoch + 59UL) / 60UL;
    char back[28];
    snprintf(back, sizeof(back), T(S_EXP_IN_FMT), (unsigned)left);
    gfx->setTextColor(0x4C98);
    gfx->setTextSize(3);
    gfx->setCursor(PET_CARD_CX - strlen(back) * 9, 86);
    gfx->print(back);
    gfx->setTextColor(UI_TRACK);
    gfx->setTextSize(1);
    gfx->setCursor(PET_CARD_CX - 90, 116);
    gfx->print(T(S_WAIT));
  } else {
    const ExpeditionSite sites[3] = { EXP_SITE_COAST, EXP_SITE_VOLCANO, EXP_SITE_FOREST };
    const int siteXs[3] = { 50, 180, 310 };
    for (uint8_t i = 0; i < 3; i++) {
      bool selected = siteChoice == sites[i];
      uint16_t col = expeditionSiteColor(sites[i]);
      gfx->fillRoundRect(siteXs[i], 60, 106, 28, 8, selected ? col : UI_WHITE);
      gfx->drawRoundRect(siteXs[i], 60, 106, 28, 8, col);
      gfx->setTextColor(selected ? UI_BG_DAY : UI_INK); gfx->setTextSize(1);
      const char *name = T(expeditionSiteText(sites[i]));
      gfx->setCursor(siteXs[i] + (106 - (int)strlen(name) * 6) / 2, 70); gfx->print(name);
    }
    char seals[20]; snprintf(seals, sizeof(seals), T(S_EXP_SEALS_FMT), pet.hoennSealCount());
    gfx->setTextColor(pet.hoennSealCount() == 3 ? UI_BAR_WARN : UI_TRACK); gfx->setTextSize(1);
    gfx->setCursor(PET_CARD_CX - (int)strlen(seals) * 3, 90); gfx->print(seals);
    const uint8_t minutes[3] = { 15, 30, 60 };
    const StrId labels[3] = { S_EXP_15, S_EXP_30, S_EXP_60 };
    const int xs[3] = { 50, 180, 310 };
    for (uint8_t i = 0; i < 3; i++) {
      uint8_t cost = Pet::expeditionEnergyCost(minutes[i]);
      bool available = pet.canStartExpedition(minutes[i], nowEpoch);
      uint16_t col = i == 0 ? UI_BAR_OK : i == 1 ? 0x4C98 : UI_BAR_BAD;
      gfx->fillRoundRect(xs[i], 100, 106, 54, 9, available ? col : UI_TRACK);
      gfx->setTextColor(UI_BG_DAY);
      gfx->setTextSize(2);
      const char *label = T(labels[i]);
      gfx->setCursor(xs[i] + (106 - (int)strlen(label) * 12) / 2, 110);
      gfx->print(label);
      char costText[12];
      snprintf(costText, sizeof(costText), "-%u ENE", cost);
      gfx->setTextSize(1);
      gfx->setCursor(xs[i] + (106 - (int)strlen(costText) * 6) / 2, 136);
      gfx->print(costText);
      char goalText[24]; snprintf(goalText, sizeof(goalText), T(S_EXP_WALK_PROGRESS), 0, Pet::expeditionStepGoal(minutes[i]));
      gfx->setCursor(xs[i] + (106 - (int)strlen(goalText) * 6) / 2, 146); gfx->print(goalText);
    }
    if (pet.expeditionInventoryFull()) {
      gfx->setTextColor(UI_BAR_BAD);
      gfx->setTextSize(1);
      gfx->setCursor(PET_CARD_CX - strlen(T(S_INV_FULL)) * 3, 164);
      gfx->print(T(S_INV_FULL));
    } else if (pet.energy < 12) {
      char need[24];
      snprintf(need, sizeof(need), T(S_NEED_ENE_FMT), 12);
      gfx->setTextColor(UI_BAR_BAD);
      gfx->setTextSize(1);
      gfx->setCursor(PET_CARD_CX - strlen(need) * 3, 164);
      gfx->print(need);
    }
  }

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_INVENTORY)) * 6, 184);
  gfx->print(T(S_INVENTORY));
  if (pet.expeditionEndEpoch && pet.expeditionMinutes) {
    char progress[48];
    if (pet.expeditionWalkBonusEarned()) snprintf(progress, sizeof(progress), "%s", T(S_EXP_WALK_EARNED));
    else snprintf(progress, sizeof(progress), T(S_EXP_WALK_PROGRESS), pet.expeditionSteps, Pet::expeditionStepGoal(pet.expeditionMinutes));
    gfx->setTextSize(1); gfx->setTextColor(pet.expeditionWalkBonusEarned() ? UI_BAR_OK : UI_INK);
    gfx->setCursor(PET_CARD_CX - (int)strlen(progress) * 3, 160); gfx->print(progress);
  }
  drawExpeditionItem(50, 200, EXP_ITEM_SNACK);
  drawExpeditionItem(244, 200, EXP_ITEM_ENERGY);
  drawExpeditionItem(50, 264, EXP_ITEM_CARE);
  drawExpeditionItem(244, 264, EXP_ITEM_TRAIN);
  if (expeditionTrainChoiceOpen) renderExpeditionTrainChoice();
}

static int8_t expeditionItemAt(int16_t x, int16_t y) {
  if ((y < 200 || y > 254) && (y < 264 || y > 318)) return -1;
  bool left = x >= 50 && x <= 222;
  bool right = x >= 244 && x <= 416;
  if (!left && !right) return -1;
  uint8_t row = y >= 264 ? 1 : 0;
  return row * 2 + (right ? 1 : 0);
}

void PetCardExpeditionScreen::handlePageTap(int16_t x, int16_t y) {
  if (expeditionTrainChoiceOpen) {
    if (y >= 172 && y <= 238 && x >= 74 && x <= 398) {
      int8_t stat = (x - 74) / 108;
      if (stat > TRAIN_STAT_SPE || !pet.useExpeditionItem(EXP_ITEM_TRAIN, stat)) sfxPlay(SFX_DENY);
      else sfxPlay(SFX_ITEM_USE);
      expeditionTrainChoiceOpen = false;
      cardDirty = true;
      lockTouchBrief();
      return;
    }
    expeditionTrainChoiceOpen = false;
    cardDirty = true;
    sfxPlay(SFX_TAP);
    return;
  }
  if (y >= 396) {
    cardOpen = false;
    markUiDirty();
    lockTouchBrief();
    sfxPlay(SFX_TAP);
    return;
  }

  uint32_t nowEpoch = expeditionNowEpoch();
  if (pet.expeditionReady(nowEpoch)) {
    if (x >= 98 && x <= 368 && y >= 98 && y <= 146) {
      ExpeditionItem item = pet.claimExpedition(nowEpoch);
      if (item != EXP_ITEM_NONE) sfxPlay(SFX_EXPEDITION_CLAIM);
      else sfxPlay(SFX_DENY);
      cardDirty = true;
      lockTouchBrief();
    }
    return;
  }
  if (!pet.expeditionActive(nowEpoch) && y >= 60 && y <= 88) {
    int idx = x >= 50 && x <= 156 ? 0 : x >= 180 && x <= 286 ? 1 : x >= 310 && x <= 416 ? 2 : -1;
    if (idx >= 0) { siteChoice = (ExpeditionSite)idx; cardDirty = true; sfxPlay(SFX_TAP); lockTouchBrief(); }
    return;
  }
  if (!pet.expeditionActive(nowEpoch) && y >= 100 && y <= 154) {
    const uint8_t minutes[3] = { 15, 30, 60 };
    int idx = x >= 50 && x <= 156 ? 0 : x >= 180 && x <= 286 ? 1 : x >= 310 && x <= 416 ? 2 : -1;
    if (idx >= 0 && pet.startExpedition(minutes[idx], nowEpoch, (uint8_t)random(100), (uint8_t)random(4), siteChoice, (uint8_t)random(100))) {
      sfxPlay(SFX_EXPEDITION_START);
      cardDirty = true;
      lockTouchBrief();
    } else {
      sfxPlay(SFX_DENY);
    }
    return;
  }

  int8_t item = expeditionItemAt(x, y);
  if (item < 0) return;
  ExpeditionItem selected = (ExpeditionItem)item;
  if (pet.itemCounts[selected] == 0) {
    sfxPlay(SFX_DENY);
    return;
  }
  if (selected == EXP_ITEM_TRAIN) {
    expeditionTrainChoiceOpen = true;
    cardDirty = true;
    sfxPlay(SFX_MENU);
  } else if (pet.useExpeditionItem(selected)) {
    cardDirty = true;
    sfxPlay(SFX_ITEM_USE);
    lockTouchBrief();
  }
}
