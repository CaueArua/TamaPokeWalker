#pragma once

#include "screen.h"

class ActivityMenuScreen : public Screen {
private:
  bool dirty = true;

public:
  /** Draws the activity menu. */
  void render() override;
  /** Opens an activity or closes the menu. */
  void handleTap(int16_t x, int16_t y) override;
  /** Returns to the main screen. */
  void handleSwipe(int dir) override;
  /** Ensures the menu is redrawn whenever it becomes active. */
  void onEnter() override { dirty = true; }
  /** Returns whether the menu can skip rendering. */
  bool isStaticClean() const override { return !dirty; }
  /** Marks the menu as needing a redraw. */
  void markDirty() override { dirty = true; }
};

extern ActivityMenuScreen activityMenuScreen;
