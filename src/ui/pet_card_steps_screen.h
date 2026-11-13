#pragma once
#include "pet_card_page.h"

class PetCardStepsScreen : public PetCardPageScreen {
public:
  void render() override;
};

extern PetCardStepsScreen petCardStepsScreen;
