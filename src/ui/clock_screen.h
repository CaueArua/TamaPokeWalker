#pragma once
#include "screen.h"

class ClockSettingsScreen : public Screen {
public:
  void render() override;
  void handleTap(int16_t x, int16_t y) override;

private:
  void applyTime();
};

extern ClockSettingsScreen clockSettingsScreen;
