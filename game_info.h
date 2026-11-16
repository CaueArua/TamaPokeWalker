#pragma once

#include <stdint.h>

#define GAME_INFO_AVAILABLE_SLOTS 30
#define GAME_INFO_NAME_LEN 13
#define GAME_INFO_REGION_LEN 16
#define GAME_INFO_ROUTE_LEN 24

/** Pokemon currently offered for capture. A dex number of 0 marks an empty slot. */
struct GameInfoAvailable {
  uint16_t dex;
  uint8_t form;
  uint8_t rarity;
  uint32_t unlockSteps;
};

/** Central game state shared by the game and its information screens. */
struct GameInfo {
  char playerName[GAME_INFO_NAME_LEN];
  uint32_t playerId;  // 0 to 999999.
  char region[GAME_INFO_REGION_LEN];
  char route[GAME_INFO_ROUTE_LEN];
  uint8_t biome;  // Biome id from biome.h (0 to BIOME_COUNT - 1).
  GameInfoAvailable available[GAME_INFO_AVAILABLE_SLOTS];
};

extern GameInfo gameInfo;

/** Loads the saved game info from NVS; call once at boot. */
void gameInfoLoad();
/** Replaces the player identity and location; text is truncated to the field sizes. Saves to NVS. */
void gameInfoSetPlayer(const char *name, uint32_t id, const char *region, const char *route);
/** Empties every available-Pokemon slot. Saves to NVS. */
void gameInfoClearAvailable();
/** Changes only the biome id; ids outside 0 to BIOME_COUNT - 1 are ignored. Saves to NVS when valid. */
bool gameInfoSetBiome(uint8_t biome);
/** Appends a Pokemon to the first empty slot; returns the slot index, or -1 when full. Saves to NVS on success. */
int8_t gameInfoAddAvailable(uint16_t dex, uint8_t form, uint8_t rarity, uint32_t unlockSteps);
