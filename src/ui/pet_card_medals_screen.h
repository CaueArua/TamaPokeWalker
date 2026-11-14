#pragma once
#include "pet_card_page.h"

class PetCardMedalsScreen : public PetCardPageScreen {
public:
  void renderPage() override;
};

extern PetCardMedalsScreen petCardMedalsScreen;
