#pragma once

#include <stdint.h>

/** Initializes the CST9217 touch controller and its interrupt. */
void touchBegin();

/** Polls touch input and dispatches completed gestures. */
void pollTouch();

/** Briefly suppresses touch input while resetting the active gesture. */
void lockTouchBrief(uint16_t durationMs = 160);

/** Returns whether a point lies inside the pet interaction area. */
bool inPetZone(int16_t x, int16_t y);

/** Returns whether a current touch or suppression window blocks light sleep. */
bool touchInputBusy(uint32_t now);

/** Records a touch interrupt detected while waking from light sleep. */
void notifyTouchWake();

/** Suppresses touch input until the supplied uptime deadline. */
void suppressTouchUntil(uint32_t deadline);
