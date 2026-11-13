#pragma once
#include "pet_card_page.h"

class PetCardProgressScreen : public PetCardPageScreen {
public:
  void render() override;
};

extern PetCardProgressScreen petCardProgressScreen;
