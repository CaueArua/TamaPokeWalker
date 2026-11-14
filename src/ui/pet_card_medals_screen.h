#pragma once
#include "pet_card_page.h"

class PetCardMedalsScreen : public PetCardPageScreen {
public:
  /** Draws earned medals and their descriptions. */
  void renderPage() override;
};

extern PetCardMedalsScreen petCardMedalsScreen;
