#pragma once
#include "pet_card_page.h"

class PetCardProgressScreen : public PetCardPageScreen {
public:
  /** Draws level, evolution, and training progress. */
  void renderPage() override;
};

extern PetCardProgressScreen petCardProgressScreen;
