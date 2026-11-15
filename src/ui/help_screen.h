#pragma once
#include "screen.h"

class HelpScreen : public Screen {
public:
  /** Resets and activates the help screen. */
  void open();
  /** Draws the current help page. */
  void render() override;
  /** Handles taps on help controls and links. */
  void handleTap(int16_t x, int16_t y) override;
  /** Changes the help page horizontally. */
  void handleSwipe(int dir) override;
  /** Handles vertical navigation or closes the help screen. */
  void handleSwipeV(int dir) override;
  /** Returns whether the current help page can skip rendering. */
  bool isStaticClean() const override;
  /** Marks the help screen as needing a redraw. */
  void markDirty() override;
};

extern HelpScreen helpScreen;
extern uint8_t helpPage;
extern bool helpDirty;
/** Returns the localized label for the selected language. */
const char *helpLabelForLanguage(uint8_t lang);
