#pragma once
#include "pet_card_page.h"

class PetCardProgressScreen : public PetCardPageScreen {
public:
  /** Draws level, evolution, and training progress. */
  void renderPage() override;

protected:
  /** Opens the evolve-or-keep dialog when the evolve button is tapped. */
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardProgressScreen petCardProgressScreen;
