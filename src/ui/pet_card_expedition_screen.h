#pragma once
#include "pet_card_page.h"
#include "../../pet.h"

class PetCardExpeditionScreen : public PetCardPageScreen {
  ExpeditionSite siteChoice = EXP_SITE_COAST;

public:
  void render() override;

protected:
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardExpeditionScreen petCardExpeditionScreen;
