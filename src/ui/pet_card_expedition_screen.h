#pragma once

#include "screen.h"
#include "../../pet.h"

class PetCardExpeditionScreen : public Screen {
  ExpeditionSite siteChoice = EXP_SITE_COAST;
  Screen *returnScreen = nullptr;
  bool dirty = true;

  /** Draws expedition status, rewards, and available sites. */
  void renderContents();
  /** Handles expedition start, claim, and item-use controls. */
  void handleExpeditionTap(int16_t x, int16_t y);
  /** Returns to the screen that opened the expedition. */
  void close();

public:
  /** Opens the expedition screen and remembers its return destination. */
  void open(Screen *returnTo);
  /** Draws expedition status and controls. */
  void render() override;
  /** Handles taps on expedition controls. */
  void handleTap(int16_t x, int16_t y) override;
  /** Closes the expedition screen on a horizontal swipe. */
  void handleSwipe(int dir) override;
  /** Closes the expedition screen on an upward swipe. */
  void handleSwipeV(int dir) override;
  /** Marks expedition content as needing a redraw. */
  void markDirty() override { dirty = true; }
  /** Returns whether the current expedition frame can be reused. */
  bool isStaticClean() const override { return !dirty; }
  /** Marks the screen dirty whenever it becomes active. */
  void onEnter() override { dirty = true; }

  void onExit() override;
};

extern PetCardExpeditionScreen petCardExpeditionScreen;
extern bool expeditionTrainChoiceOpen;
