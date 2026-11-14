#pragma once
#include "screen.h"

static constexpr uint8_t PET_CARD_COUNT = 9;

class PetCardPageScreen : public Screen {
public:
  void render() final;
  void handleTap(int16_t x, int16_t y) final;
  void handleSwipe(int dir) final;
  void handleSwipeV(int dir) final;
  bool isStaticClean() const override;
  void markDirty() override;

protected:
  virtual void renderPage() = 0;
  virtual void handlePageTap(int16_t, int16_t) {}
};

PetCardPageScreen *petCardScreenForPage(uint8_t page);
