#pragma once
#include "pet_card_page.h"

class PetCardBoxScreen : public PetCardPageScreen {
  static constexpr uint8_t ROWS = 5;
  uint8_t page = 0;
  uint8_t sort = 0;

  bool comesBefore(int16_t a, int16_t b) const;
  uint16_t buildList(int16_t *out) const;
  uint8_t pageCount() const;
  int16_t dexAt(uint16_t index) const;
  const char *sortLabel() const;

public:
  void renderPage() override;

protected:
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardBoxScreen petCardBoxScreen;
