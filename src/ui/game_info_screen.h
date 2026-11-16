#pragma once
#include "screen.h"

class GameInfoScreen : public Screen {
  Screen *returnScreen = nullptr;
  bool dirty = true;

  /** Returns to the screen that opened this one. */
  void close();

public:
  /** Activates the read-only game information screen; a bottom tap or swipe returns to returnTo. */
  void open(Screen *returnTo);
  /** Draws the player, ID, region, route and biome. */
  void render() override;
  /** Handles a vertical swipe by returning to the caller. */
  void handleSwipeV(int dir) override;
  /** Returns to the caller when the tap lands in the bottom band. */
  void handleTap(int16_t x, int16_t y) override;
  /** Returns whether the screen can skip rendering. */
  bool isStaticClean() const override;
  /** Marks the screen as needing a redraw. */
  void markDirty() override;
};

extern GameInfoScreen gameInfoScreen;
