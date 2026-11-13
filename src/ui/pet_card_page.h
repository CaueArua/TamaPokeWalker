#pragma once
#include "screen.h"

class PetCardPageScreen : public Screen {
public:
  void handleTap(int16_t x, int16_t y) final {
    handlePageTap(x, y);
  }

protected:
  virtual void handlePageTap(int16_t, int16_t) {}
};
