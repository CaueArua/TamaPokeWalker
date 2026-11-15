#pragma once

#include <stdint.h>

#include "../../sdmon.h"

/** Interpolates between two RGB565 colors. */
uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);

/** Draws an encoded sprite thumbnail centered in its cell. */
void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);

/** Returns the duration of a PMD animation action. */
uint32_t pmdActTotalMs(const PmdAct &action);

/** Selects the PMD animation frame at the requested elapsed time. */
uint8_t pmdFrameAt(const PmdAct &action, uint32_t elapsedMs, bool loop);

/** Draws an action from a specific PMD sprite. */
void drawPmdActM(PmdMon &mon, uint8_t action, int cx, int groundY,
                 uint32_t elapsedMs, bool loop, bool silhouette, uint8_t maxScale);

/** Draws an action from the current pet's PMD sprite. */
void drawPmdAct(uint8_t action, int cx, int groundY, uint32_t elapsedMs,
                bool loop, bool silhouette, uint8_t maxScale);

/** Draws an n-by-n character sprite map at an integer scale. */
void drawMap(const char *const *map, int size, int x, int y, int scale, bool silhouette);
