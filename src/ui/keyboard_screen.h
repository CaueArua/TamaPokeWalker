#pragma once
#include "screen.h"

class KeyboardScreen : public Screen {
public:
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
};

extern KeyboardScreen keyboardScreen;
