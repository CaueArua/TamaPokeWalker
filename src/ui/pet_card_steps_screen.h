#pragma once
#include "pet_card_page.h"

class PetCardStepsScreen : public PetCardPageScreen {
public:
  void renderPage() override;
};

extern PetCardStepsScreen petCardStepsScreen;
