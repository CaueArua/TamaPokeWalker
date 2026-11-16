#include "game_info.h"

#include <Preferences.h>
#include <string.h>

#include "biome.h"

GameInfo gameInfo = {};

static void copyText(char *dst, size_t size, const char *src) {
  strncpy(dst, src, size - 1);
  dst[size - 1] = '\0';
}

static void gameInfoSave() {
  Preferences p;
  p.begin("gameinfo", false);
  p.putString("name", gameInfo.playerName);
  p.putUInt("id", gameInfo.playerId);
  p.putString("region", gameInfo.region);
  p.putString("route", gameInfo.route);
  p.putUChar("biome", gameInfo.biome);
  p.putBytes("avail", gameInfo.available, sizeof(gameInfo.available));
  p.end();
}

void gameInfoLoad() {
  Preferences p;
  p.begin("gameinfo", true);
  p.getString("name", gameInfo.playerName, sizeof(gameInfo.playerName));
  gameInfo.playerId = p.getUInt("id", 0);
  p.getString("region", gameInfo.region, sizeof(gameInfo.region));
  p.getString("route", gameInfo.route, sizeof(gameInfo.route));
  uint8_t biome = p.getUChar("biome", 0);
  gameInfo.biome = biome < BIOME_COUNT ? biome : 0;
  if (p.getBytesLength("avail") == sizeof(gameInfo.available)) {
    p.getBytes("avail", gameInfo.available, sizeof(gameInfo.available));
  }
  p.end();
}

void gameInfoSetPlayer(const char *name, uint32_t id, const char *region, const char *route) {
  copyText(gameInfo.playerName, sizeof(gameInfo.playerName), name);
  gameInfo.playerId = id;
  copyText(gameInfo.region, sizeof(gameInfo.region), region);
  copyText(gameInfo.route, sizeof(gameInfo.route), route);
  gameInfoSave();
}

void gameInfoClearAvailable() {
  memset(gameInfo.available, 0, sizeof(gameInfo.available));
  gameInfoSave();
}

bool gameInfoSetBiome(uint8_t biome) {
  if (biome >= BIOME_COUNT) return false;
  gameInfo.biome = biome;
  gameInfoSave();
  return true;
}

int8_t gameInfoAddAvailable(uint16_t dex, uint8_t form, uint8_t rarity, uint32_t unlockSteps) {
  for (int i = 0; i < GAME_INFO_AVAILABLE_SLOTS; ++i) {
    GameInfoAvailable &slot = gameInfo.available[i];
    if (slot.dex != 0) continue;
    slot.dex = dex;
    slot.form = form;
    slot.rarity = rarity;
    slot.unlockSteps = unlockSteps;
    gameInfoSave();
    return (int8_t)i;
  }
  return -1;
}
