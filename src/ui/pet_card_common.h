#pragma once
#include <stdint.h>

#ifndef C565
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#endif

static constexpr int PET_CARD_CX = 233;

/** Draws a labeled stat value and its proportional bar on a pet card. */
void drawPetCardStat(int y, const char *label, uint16_t val, uint16_t maxBar, uint16_t color);
