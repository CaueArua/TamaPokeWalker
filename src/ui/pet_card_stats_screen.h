#pragma once
#include "pet_card_page.h"

class PetCardStatsScreen : public PetCardPageScreen {
public:
  /** Draws the pet's battle stats and training controls. */
  void renderPage() override;

protected:
  /** Handles taps on the battle and training controls. */
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardStatsScreen petCardStatsScreen;
