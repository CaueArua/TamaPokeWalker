#pragma once

#include <stdint.h>
#include <FS.h> // Interface abstrata (aceita FFat, SD, LittleFS, etc.)

/** Base stats, mirroring the "stats" object of the source JSON. */
struct NationalDexStats {
  uint8_t hp;
  uint8_t attack;
  uint8_t defense;
  uint8_t specialAttack;
  uint8_t specialDefense;
  uint8_t speed;
};

/** One way to trigger an evolution. minLevel is 0 when the method has no level requirement. */
struct NationalDexEvolutionMethod {
  const char *trigger;
  uint8_t minLevel;
};

/** A possible evolution target and the methods that trigger it. */
struct NationalDexEvolution {
  uint16_t id;
  uint8_t methodCount;
  const NationalDexEvolutionMethod *methods;
};

/** Full record of one Pokemon, dynamically loaded into PSRAM from JSON. */
struct NationalDexEntry {
  uint16_t id;
  const char *name;
  uint8_t typeCount;
  const char *const *types;
  const char *growthRate;
  uint8_t eggGroupCount;
  const char *const *eggGroups;
  NationalDexStats stats;
  uint8_t evolutionCount;
  const NationalDexEvolution *evolvesTo;
};

/** Loads the National Dex from a JSON file on any storage backend (FFat, SD, LittleFS) into PSRAM. */
bool nationalDexLoad(fs::FS &storage, const char *path = "/mons/national_dex.json");

/** Frees all PSRAM allocated by the National Dex. */
void nationalDexUnload();

/** Returns the number of loaded Pokemon entries. */
uint16_t nationalDexCount();

/** Returns the full record for a national dex id, or the MissingNo record (id 0) when the id is unknown. */
const NationalDexEntry *nationalDexFind(uint16_t id);

/** Returns true and sets out/count to the evolution list of the id. Returns false when the id is unknown. */
bool nationalDexGetEvolvesTo(uint16_t id, const NationalDexEvolution **out, uint8_t *count);

/** Returns true and copies the base stats of the id into out. Returns false when the id is unknown. */
bool nationalDexGetStats(uint16_t id, NationalDexStats *out);

/** Returns true and sets out/count to the type names of the id. Returns false when the id is unknown. */
bool nationalDexGetTypes(uint16_t id, const char *const **out, uint8_t *count);

/** Returns true and sets out to the growth rate of the id. Returns false when the id is unknown. */
bool nationalDexGetGrowthRate(uint16_t id, const char **out);
