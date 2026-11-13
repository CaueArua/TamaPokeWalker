#pragma once
#include "screen.h"

class GameCleanScreen : public Screen {
public:
  bool isStaticClean() const override { return false; }
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
};

extern GameCleanScreen gameCleanScreen;
