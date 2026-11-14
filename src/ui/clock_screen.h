#pragma once
#include "screen.h"

class ClockSettingsScreen : public Screen {
  Screen *returnScreen = nullptr;

  /** Closes the settings screen and returns to its caller. */
  void closeSettings();

public:
  /** Sets the screen to return to when the settings screen closes. */
  void setReturnScreen(Screen *screen) { returnScreen = screen; }
  /** Draws the clock and language settings. */
  void render() override;
  /** Handles taps on the clock and language controls. */
  void handleTap(int16_t x, int16_t y) override;
  /** Handles a vertical swipe to close the settings screen. */
  void handleSwipeV(int dir) override;
  /** Returns whether the settings screen can skip rendering. */
  bool isStaticClean() const override;
  /** Marks the settings screen as needing a redraw. */
  void markDirty() override;

private:
  /** Applies the selected clock values to the RTC and pet state. */
  void applyTime();
};

extern ClockSettingsScreen clockSettingsScreen;
