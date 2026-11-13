#pragma once
#include "pet_card_page.h"

class PetCardMedalsScreen : public PetCardPageScreen {
public:
  void render() override;
};

extern PetCardMedalsScreen petCardMedalsScreen;
