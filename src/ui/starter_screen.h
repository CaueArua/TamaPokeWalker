#pragma once
#include "screen.h"

class StarterScreen : public Screen {
private:
  uint8_t region = 0xFF;
  bool dirty = true;

public:
  void onEnter() override;
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  bool isStaticClean() const override { return !dirty; }
  void markDirty() override { dirty = true; }
};

extern StarterScreen starterScreen;
