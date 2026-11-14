#pragma once
#include "screen.h"

class StarterScreen : public Screen {
private:
  uint8_t region = 0xFF;
  bool dirty = true;

public:
  /** Resets selection state when the starter screen becomes active. */
  void onEnter() override;
  /** Draws the region and starter selection interface. */
  void render() override;
  /** Handles selection of a region or starter. */
  void handleTap(int16_t x, int16_t y) override;
  /** Returns whether the screen can skip rendering. */
  bool isStaticClean() const override { return !dirty; }
  /** Marks the starter screen as needing a redraw. */
  void markDirty() override { dirty = true; }
};

extern StarterScreen starterScreen;
