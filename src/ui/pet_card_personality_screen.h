#pragma once
#include "pet_card_page.h"

class PetCardPersonalityScreen : public PetCardPageScreen {
public:
  void render() override;
};

extern PetCardPersonalityScreen petCardPersonalityScreen;
