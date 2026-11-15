#pragma once
#include "screen.h"

class MenuScreen : public Screen {
private:
  bool dirty = true;

public:
  void markDirty() override { dirty = true; }
  bool isStaticClean() const override { return !dirty; }

  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  void handleSwipe(int dir) override;
};

extern MenuScreen menuScreen;
