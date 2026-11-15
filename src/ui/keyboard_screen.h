#pragma once
#include "screen.h"

class KeyboardScreen : public Screen {
  Screen *returnScreen = nullptr;

public:
  /** Initializes the rename buffer and activates the keyboard screen. */
  void open(Screen *returnTo);
  /** Draws the rename keyboard and current text buffer. */
  void render() override;
  /** Handles keyboard key presses and rename controls. */
  void handleTap(int16_t x, int16_t y) override;
  /** Returns whether the keyboard can skip rendering. */
  bool isStaticClean() const override;
  /** Marks the keyboard as needing a redraw. */
  void markDirty() override;
};

extern KeyboardScreen keyboardScreen;
extern char nameBuf[12];
extern uint8_t nameLen;
extern bool keyboardDirty;
