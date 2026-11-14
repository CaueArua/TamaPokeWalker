#pragma once
#include <Arduino.h>
#include <stdint.h>

class Screen {
public:
  /** Provides a virtual destructor for safe polymorphic screen use. */
  virtual ~Screen() {}

  /** Runs once when the screen becomes active. */
  virtual void onEnter() {}
  /** Runs once when the screen stops being active. */
  virtual void onExit() {}

  /** Draws the screen and submits its completed frame. */
  virtual void render() = 0;

  /** Handles a tap at the supplied display coordinates. */
  virtual void handleTap(int16_t x, int16_t y) {}
  /** Handles the press phase of a touch gesture. */
  virtual void handlePress(int16_t y) {}
  /** Returns the hold duration required to trigger a long press. */
  virtual uint32_t longPressDelayMs() const { return 700; }
  /** Handles a completed long press at the supplied coordinates. */
  virtual void handleLongPress(int16_t x, int16_t y, uint32_t durationMs) {}
  /** Handles a horizontal swipe; -1 is left and +1 is right. */
  virtual void handleSwipe(int dir) {}
  /** Handles a vertical swipe; -1 is up and +1 is down. */
  virtual void handleSwipeV(int dir) {}
  /** Returns whether taps should be delivered on press rather than release. */
  virtual bool handlesTapOnPress() const { return false; }
  /** Returns whether this screen represents a minigame. */
  virtual bool isMinigame() const { return false; }
  /** Returns whether this screen is one of the pet-card pages. */
  virtual bool isPetCardPage() const { return false; }
  /** Returns whether the screen has requested to close. */
  virtual bool shouldClose() const { return false; }

  /** Returns whether a static screen can skip rendering because it is clean. */
  virtual bool isStaticClean() const { return false; }
  /** Marks screen content as needing a redraw. */
  virtual void markDirty() {}
};

/** Active modular screen, or null while the main screen is active. */
extern Screen* activeScreen;

/** Exits the current screen and activates the requested screen. */
void setScreen(Screen* next);
