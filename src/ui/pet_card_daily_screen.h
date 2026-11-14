#pragma once
#include "pet_card_page.h"

class PetCardDailyScreen : public PetCardPageScreen {
public:
  /** Draws daily goals and their completion state. */
  void renderPage() override;
};

extern PetCardDailyScreen petCardDailyScreen;
