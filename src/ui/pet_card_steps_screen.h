#pragma once
#include "pet_card_page.h"

class PetCardStepsScreen : public PetCardPageScreen {
public:
  /** Draws step totals, trail rank, and walking rewards. */
  void renderPage() override;
};

extern PetCardStepsScreen petCardStepsScreen;
