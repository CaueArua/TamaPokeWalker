#pragma once
#include "pet_card_page.h"

class PetCardDailyScreen : public PetCardPageScreen {
public:
  void renderPage() override;
};

extern PetCardDailyScreen petCardDailyScreen;
