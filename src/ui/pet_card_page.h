#pragma once
#include "screen.h"

/** Pet-card page indices. Pages from PET_CARD_SWIPE_COUNT on are detail pages reached only by buttons. */
static constexpr uint8_t PET_CARD_PAGE_SUMMARY = 0;
static constexpr uint8_t PET_CARD_PAGE_INFOS = 1;
static constexpr uint8_t PET_CARD_PAGE_PERSONALITY = 2;
static constexpr uint8_t PET_CARD_PAGE_DAILY = 3;
static constexpr uint8_t PET_CARD_PAGE_BOX = 4;
static constexpr uint8_t PET_CARD_PAGE_STEPS = 5;
static constexpr uint8_t PET_CARD_PAGE_STATS = 6;
static constexpr uint8_t PET_CARD_PAGE_MEDALS = 7;
static constexpr uint8_t PET_CARD_PAGE_PROGRESS = 8;
static constexpr uint8_t PET_CARD_SWIPE_COUNT = 6;
static constexpr uint8_t PET_CARD_COUNT = 9;
extern uint8_t cardPage;
extern bool cardDirty;


class PetCardPageScreen : public Screen {
public:
  /** Draws the shared pet-card frame and the subclass page content. */
  void render() final;
  /** Routes taps to the current card page. */
  void handleTap(int16_t x, int16_t y) final;
  /** Changes to the adjacent pet-card page. */
  void handleSwipe(int dir) final;
  /** Handles vertical card navigation or closes the card. */
  void handleSwipeV(int dir) final;
  /** Identifies this screen as a pet-card page. */
  bool isPetCardPage() const override { return true; }
  /** Returns whether the current card page can skip rendering. */
  bool isStaticClean() const override;
  /** Marks the current card page as needing a redraw. */
  void markDirty() override;

protected:
  /** Draws the content specific to the derived card page. */
  virtual void renderPage() = 0;
  /** Handles a tap on content specific to the derived card page. */
  virtual void handlePageTap(int16_t, int16_t) {}
};

/** Returns the screen instance associated with a pet-card page number. */
PetCardPageScreen *petCardScreenForPage(uint8_t page);
/** Opens a card page with an optional screen to return to when it closes. */
void openPetCardPage(uint8_t page, Screen *returnTo = nullptr);
/** Opens a card page from inside the card, keeping the current return target. */
void openPetCardSubpage(uint8_t page);
