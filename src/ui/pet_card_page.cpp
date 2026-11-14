#include "pet_card_page.h"
#include "pet_card_common.h"
#include "pet_card_profile_screen.h"
#include "pet_card_personality_screen.h"
#include "pet_card_daily_screen.h"
#include "pet_card_box_screen.h"
#include "pet_card_stats_screen.h"
#include "pet_card_medals_screen.h"
#include "pet_card_progress_screen.h"
#include "pet_card_expedition_screen.h"
#include "pet_card_steps_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../i18n.h"
#include "../../audio.h"

extern Arduino_Canvas *gfx;
extern uint8_t cardPage;
extern bool cardOpen;
extern bool cardDirty;
extern bool expeditionTrainChoiceOpen;
extern void flushStampedFrame();
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);

PetCardPageScreen *petCardScreenForPage(uint8_t page) {
  switch (page) {
    case 0: return &petCardProfileScreen;
    case 1: return &petCardPersonalityScreen;
    case 2: return &petCardDailyScreen;
    case 3: return &petCardBoxScreen;
    case 4: return &petCardStatsScreen;
    case 5: return &petCardMedalsScreen;
    case 6: return &petCardProgressScreen;
    case 7: return &petCardExpeditionScreen;
    default: return &petCardStepsScreen;
  }
}

void PetCardPageScreen::render() {
  cardDirty = false;
  gfx->fillScreen(UI_BG_DAY);
  renderPage();

  int dotsX = PET_CARD_CX - ((PET_CARD_COUNT - 1) * 24) / 2;
  for (int i = 0; i < PET_CARD_COUNT; i++) {
    if (i == cardPage) gfx->fillCircle(dotsX + i * 24, 400, 5, UI_INK);
    else gfx->drawCircle(dotsX + i * 24, 400, 4, UI_INK);
  }
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_BACK)) * 6, 420);
  gfx->print(T(S_BACK));
  flushStampedFrame();
}

void PetCardPageScreen::handleTap(int16_t x, int16_t y) {
  handlePageTap(x, y);
  if (activeScreen != this) return;
  if (!cardOpen) {
    setScreen(nullptr);
    return;
  }
  if (y >= 400) {
    cardOpen = false;
    markUiDirty();
    lockTouchBrief();
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
  }
}

void PetCardPageScreen::handleSwipe(int dir) {
  int next = (int)cardPage + (dir > 0 ? -1 : 1);
  uint8_t old = cardPage;
  cardPage = next < 0 ? 0 : (next >= PET_CARD_COUNT ? PET_CARD_COUNT - 1 : next);
  if (cardPage != old) {
    expeditionTrainChoiceOpen = false;
    cardDirty = true;
    sfxPlay(SFX_MENU);
    setScreen(petCardScreenForPage(cardPage));
  }
}

void PetCardPageScreen::handleSwipeV(int dir) {
  if (dir < 0) {
    cardOpen = false;
    expeditionTrainChoiceOpen = false;
    markUiDirty();
    lockTouchBrief();
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
  }
}

bool PetCardPageScreen::isStaticClean() const {
  return !cardDirty;
}

void PetCardPageScreen::markDirty() {
  cardDirty = true;
}
