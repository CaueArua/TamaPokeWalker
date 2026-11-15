#pragma once
#include "pet_card_page.h"

class PetCardSummaryScreen : public PetCardPageScreen {
public:
  /** Draws the animated pet with its name and the four detail buttons. */
  void renderPage() override;

protected:
  /** Opens the detail page whose button was tapped. */
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardSummaryScreen petCardSummaryScreen;
