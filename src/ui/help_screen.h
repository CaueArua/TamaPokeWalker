#pragma once
#include "screen.h"

class HelpScreen : public Screen {
public:
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  void handleSwipe(int dir) override;
  void handleSwipeV(int dir) override;
};

extern HelpScreen helpScreen;
const char *helpLabelForLanguage(uint8_t lang);
