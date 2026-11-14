#pragma once
#include "pet_card_page.h"

class PetCardPersonalityScreen : public PetCardPageScreen {
public:
  /** Draws the pet's personality and its description. */
  void renderPage() override;
};

extern PetCardPersonalityScreen petCardPersonalityScreen;
