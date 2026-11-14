#pragma once
#include "pet_card_page.h"

class PetCardBoxScreen : public PetCardPageScreen {
  static constexpr uint8_t ROWS = 5;
  uint8_t page = 0;
  uint8_t sort = 0;

  /** Compares two species using the active box sort order. */
  bool comesBefore(int16_t a, int16_t b) const;
  /** Builds the filtered, sorted box list into the supplied buffer. */
  uint16_t buildList(int16_t *out) const;
  /** Returns the number of pages in the current box list. */
  uint8_t pageCount() const;
  /** Returns the species at an index in the current box list. */
  int16_t dexAt(uint16_t index) const;
  /** Returns the localized label for the active sort order. */
  const char *sortLabel() const;

public:
  /** Draws the current page of the captured-species box. */
  void renderPage() override;

protected:
  /** Handles box paging, sorting, and species-detail taps. */
  void handlePageTap(int16_t x, int16_t y) override;
};

extern PetCardBoxScreen petCardBoxScreen;
