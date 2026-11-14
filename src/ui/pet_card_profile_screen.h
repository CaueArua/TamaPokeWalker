#pragma once
#include "pet_card_page.h"

class PetCardProfileScreen : public PetCardPageScreen {
public:
  /** Draws the pet's profile and care summary. */
  void renderPage() override;

protected:
  /** Handles profile actions such as opening the rename keyboard. */
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardProfileScreen petCardProfileScreen;
