#pragma once
#include "pet_card_page.h"

class PetCardStatsScreen : public PetCardPageScreen {
public:
  void renderPage() override;

protected:
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardStatsScreen petCardStatsScreen;
