#pragma once

#include <stdint.h>

#include "../../i18n.h"

extern const uint16_t BIOME_SOIL[6];
extern const uint16_t STARS[6][2];
extern bool gNight;

/** Returns the foreground ink color for the current day/night scene. */
uint16_t inkColor();

/** Indicates whether the watermark has moved and requires a redraw. */
bool watermarkNeedsRedraw();

/** Stamps the current frame with a watermark and submits it to the display. */
void flushStampedFrame();

/** Returns the pet's current local-scene hour. */
int sceneHour();

/** Returns the current day-phase category. */
uint8_t currentDayPhase();

/** Maps a day-phase identifier to its localized text resource. */
StrId dayPhaseTextId(uint8_t phase);

/** Draws the shared minigame background using the current hour and biome. */
void drawGameScene();
