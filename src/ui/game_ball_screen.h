#pragma once
#include "screen.h"

class GameBallScreen : public Screen {
public:
  bool isStaticClean() const override { return false; }
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
};

extern GameBallScreen gameBallScreen;
