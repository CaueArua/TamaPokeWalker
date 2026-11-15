#pragma once

#include "screen.h"

class SettingsScreen : public Screen {
  uint8_t page = 0;
  uint8_t brightness = 100;
  bool wifi = false;
  bool dirty = true;

public:
  void open();
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  void handleSwipe(int direction) override;
  void handleSwipeV(int direction) override;
  bool isStaticClean() const override { return !dirty; }
  void markDirty() override { dirty = true; }
};

extern SettingsScreen settingsScreen;
