#include "pet_card_page.h"
#include "pet_card_common.h"
#include "pet_card_summary_screen.h"
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
#include "touch_controller.h"

extern Arduino_Canvas *gfx;
extern bool expeditionTrainChoiceOpen;
extern void flushStampedFrame();

uint8_t cardPage = 0;
bool cardDirty = true;
static Screen *cardReturnScreen = nullptr;

/** Returns whether a page is a detail page that is reached only through the summary buttons. */
static bool isDetailPage(uint8_t page) {
  return page >= PET_CARD_SWIPE_COUNT;
}

PetCardPageScreen *petCardScreenForPage(uint8_t page) {
  switch (page) {
    case PET_CARD_PAGE_SUMMARY: return &petCardSummaryScreen;
    case PET_CARD_PAGE_INFOS: return &petCardProfileScreen;
    case PET_CARD_PAGE_PERSONALITY: return &petCardPersonalityScreen;
    case PET_CARD_PAGE_DAILY: return &petCardDailyScreen;
    case PET_CARD_PAGE_BOX: return &petCardBoxScreen;
    case PET_CARD_PAGE_STEPS: return &petCardStepsScreen;
    case PET_CARD_PAGE_STATS: return &petCardStatsScreen;
    case PET_CARD_PAGE_MEDALS: return &petCardMedalsScreen;
    case PET_CARD_PAGE_PROGRESS: return &petCardProgressScreen;
    default: return &petCardSummaryScreen;
  }
}

void openPetCardPage(uint8_t page, Screen *returnTo) {
  if (page >= PET_CARD_COUNT) return;
  cardPage = page;
  cardDirty = true;
  cardReturnScreen = returnTo;
  setScreen(petCardScreenForPage(page));
}

void openPetCardSubpage(uint8_t page) {
  if (page >= PET_CARD_COUNT) return;
  cardPage = page;
  cardDirty = true;
  setScreen(petCardScreenForPage(page));
}

static void closePetCardPages() {
  Screen *target = cardReturnScreen;
  cardReturnScreen = nullptr;
  setScreen(target);
}

void PetCardPageScreen::render() {
  cardDirty = false;
  gfx->fillScreen(UI_BG_DAY);
  renderPage();

  if (showPageDots()) {
    int dotsX = PET_CARD_CX - ((PET_CARD_SWIPE_COUNT - 1) * 24) / 2;
    for (int i = 0; i < PET_CARD_SWIPE_COUNT; i++) {
      if (i == cardPage) gfx->fillCircle(dotsX + i * 24, 400, 5, UI_INK);
      else gfx->drawCircle(dotsX + i * 24, 400, 4, UI_INK);
    }
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
  if (y >= 400) {
    if (isDetailPage(cardPage)) {
      openPetCardSubpage(PET_CARD_PAGE_SUMMARY);
    } else {
      closePetCardPages();
    }
    lockTouchBrief();
    sfxPlay(SFX_TAP);
  }
}

void PetCardPageScreen::handleSwipe(int dir) {
  if (isDetailPage(cardPage)) return;
  int next = (int)cardPage + (dir > 0 ? -1 : 1);
  uint8_t old = cardPage;
  cardPage = next < 0 ? 0 : (next >= PET_CARD_SWIPE_COUNT ? PET_CARD_SWIPE_COUNT - 1 : next);
  if (cardPage != old) {
    expeditionTrainChoiceOpen = false;
    cardDirty = true;
    sfxPlay(SFX_MENU);
    setScreen(petCardScreenForPage(cardPage));
  }
}


void PetCardPageScreen::handleSwipeV(int dir) {
  if (dir < 0) {
    expeditionTrainChoiceOpen = false;
    closePetCardPages();
    lockTouchBrief();
    sfxPlay(SFX_TAP);
  }
}

bool PetCardPageScreen::isStaticClean() const {
  return !cardDirty;
}

void PetCardPageScreen::markDirty() {
  cardDirty = true;
}
