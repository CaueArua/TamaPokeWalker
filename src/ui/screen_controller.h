#pragma once

#include "screen.h"

/** Returns the diagnostic identifier for the active screen. */
const char *screenName();

/** Returns the minimum interval between rendered frames. */
uint16_t renderIntervalMs();

/** Renders the active screen, or the main screen when no modular screen is active. */
void render();

/** Routes a vertical swipe to the active screen or the home screen. */
void onSwipeV(int direction);

/** Routes a horizontal swipe to the active screen or the home screen. */
void onSwipe(int direction);

/** Routes a tap to the active screen or the home screen. */
void onTap(int16_t x, int16_t y);
