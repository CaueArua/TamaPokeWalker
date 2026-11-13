#pragma once
#include "pet_card_page.h"

class PetCardDailyScreen : public PetCardPageScreen {
public:
  void render() override;
};

extern PetCardDailyScreen petCardDailyScreen;
