#include "nationalDex.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <string.h>

// =========================================================================
// REGISTRO FALLBACK: MISSINGNO. (ID 0)
// =========================================================================

static const char *const kMissingNoTypes[] = {"normal"};
static const NationalDexStats kMissingNoStats = {10, 10, 10, 10, 10, 10};

static const NationalDexEntry kMissingNo = {
    0,                 // id
    "MissingNo",       // name
    1,                 // typeCount
    kMissingNoTypes,   // types
    "medium-fast",     // growthRate
    0,                 // eggGroupCount
    nullptr,           // eggGroups
    kMissingNoStats,   // stats (todos em 10)
    0,                 // evolutionCount
    nullptr            // evolvesTo
};

// =========================================================================
// ALOCADORES PSRAM PARA ARDUINOJSON E DADOS DA DEX
// =========================================================================

struct SpiRamAllocator : ArduinoJson::Allocator {
  void *allocate(size_t size) override {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  void deallocate(void *pointer) override {
    heap_caps_free(pointer);
  }
  void *reallocate(void *ptr, size_t new_size) override {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
};

struct Arena {
  uint8_t *base = nullptr;
  size_t cap = 0;
  size_t used = 0;
};

// sStrings guarda apenas textos terminados em '\0', para que internString leia somente texto.
// sData guarda listas e structs binarias (ponteiros, evolucoes, metodos).
static Arena sStrings;
static Arena sData;

static NationalDexEntry *sEntries = nullptr;
static uint16_t sEntryCount = 0;

static bool arenaInit(Arena &a, size_t bytes) {
  a.base = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  a.cap = a.base ? bytes : 0;
  a.used = 0;
  return a.base != nullptr;
}

static void arenaFree(Arena &a) {
  if (a.base) heap_caps_free(a.base);
  a = Arena();
}

static void *arenaAlloc(Arena &a, size_t bytes, size_t align = 4) {
  size_t offset = (a.used + (align - 1)) & ~(align - 1);
  if (!a.base || offset + bytes > a.cap) return nullptr;
  a.used = offset + bytes;
  return a.base + offset;
}

static const char *internString(const char *str) {
  if (!str) str = "";
  size_t len = strlen(str) + 1;

  size_t pos = 0;
  while (pos < sStrings.used) {
    const char *existing = (const char *)(sStrings.base + pos);
    size_t elen = strlen(existing) + 1;
    if (elen == len && memcmp(existing, str, len) == 0) {
      return existing;
    }
    pos += elen;
  }

  char *dst = (char *)arenaAlloc(sStrings, len, 1);
  if (!dst) return nullptr;
  memcpy(dst, str, len);
  return dst;
}

// =========================================================================
// CARREGAMENTO DO JSON
// =========================================================================

void nationalDexUnload() {
  if (sEntries) {
    heap_caps_free(sEntries);
    sEntries = nullptr;
  }
  arenaFree(sStrings);
  arenaFree(sData);
  sEntryCount = 0;
}

static bool loadFailed(const char *motivo) {
  Serial.printf("[NationalDex] %s. Usando fallback MissingNo.\n", motivo);
  nationalDexUnload();
  return false;
}

struct ArenaSize {
  size_t strings;
  size_t data;
};

// Limite superior do espaco necessario. Textos repetidos ocupam menos por causa do internString.
static ArenaSize measureArena(JsonArray arr) {
  const size_t kAlignSlack = 8;
  ArenaSize s = {0, 0};
  for (JsonObject obj : arr) {
    s.strings += strlen(obj["name"] | "MissingNo") + 1;
    s.strings += strlen(obj["growth_rate"] | "medium-fast") + 1;

    JsonArray types = obj["types"].as<JsonArray>();
    if (!types.isNull() && types.size() > 0) {
      s.data += sizeof(char *) * types.size() + kAlignSlack;
      for (JsonVariant t : types) s.strings += strlen(t | "normal") + 1;
    }

    JsonArray eggs = obj["egg_groups"].as<JsonArray>();
    if (!eggs.isNull() && eggs.size() > 0) {
      s.data += sizeof(char *) * eggs.size() + kAlignSlack;
      for (JsonVariant g : eggs) s.strings += strlen(g | "undiscovered") + 1;
    }

    JsonArray evos = obj["evolves_to"].as<JsonArray>();
    if (!evos.isNull() && evos.size() > 0) {
      s.data += sizeof(NationalDexEvolution) * evos.size() + kAlignSlack;
      for (JsonObject evo : evos) {
        JsonArray methods = evo["methods"].as<JsonArray>();
        if (!methods.isNull() && methods.size() > 0) {
          s.data += sizeof(NationalDexEvolutionMethod) * methods.size() + kAlignSlack;
          for (JsonObject m : methods) s.strings += strlen(m["trigger"] | "level-up") + 1;
        }
      }
    }
  }
  return s;
}

bool nationalDexLoad(fs::FS &storage, const char *path) {
  // Serial.println("[NationalDex], Descarregando National Dex existente...");
  nationalDexUnload();

  // Serial.printf("[NationalDex], Iniciando carga do National Dex...\n");

  File file = storage.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    Serial.printf("[NationalDex] Arquivo nao encontrado (%s). Usando fallback MissingNo.\n", path);
    return false;
  }

  size_t fileSize = file.size();
  if (fileSize == 0) {
    file.close();
    Serial.println("[NationalDex] Arquivo vazio. Usando fallback MissingNo.");
    return false;
  }

  // Serial.printf("[NationalDex], Tamanho do arquivo: %u bytes\n", fileSize);

  SpiRamAllocator allocator;
  JsonDocument doc(&allocator);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  // Serial.println("[NationalDex], JSON lido do arquivo.");

  if (err) {
    Serial.printf("[NationalDex] Erro ao ler JSON (%s). Usando fallback MissingNo.\n", err.c_str());
    return false;
  }

  JsonArray arr = doc.is<JsonArray>() ? doc.as<JsonArray>() : doc["pokemon"].as<JsonArray>();
  if (arr.isNull() || arr.size() == 0) {
    Serial.println("[NationalDex] JSON sem entradas. Usando fallback MissingNo.");
    return false;
  }

  
  uint16_t count = (uint16_t)arr.size();
  // Serial.printf("[NationalDex], Contagem de entradas: %u\n", count);
  ArenaSize need = measureArena(arr);
  sEntries = (NationalDexEntry *)heap_caps_calloc(count, sizeof(NationalDexEntry),
                                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!sEntries || !arenaInit(sStrings, need.strings) || !arenaInit(sData, need.data)) {
    return loadFailed("Falha ao alocar PSRAM");
  }

  uint16_t idx = 0;
  // Serial.println("[NationalDex], Iniciando processamento das entradas...");
  for (JsonObject obj : arr) {
    // Serial.printf("[NationalDex], Processando entrada %u...\n", idx);
    NationalDexEntry &e = sEntries[idx++];
    e.id = obj["id"] | 0;
    const char *name = internString(obj["name"] | "MissingNo");
    const char *growth = internString(obj["growth_rate"] | "medium-fast");
    if (!name || !growth) return loadFailed("Arena de textos esgotada");
    e.name = name;
    e.growthRate = growth;

    // 1. Types
    JsonArray typesArr = obj["types"].as<JsonArray>();
    e.typeCount = typesArr.isNull() ? 0 : (uint8_t)typesArr.size();
    if (e.typeCount > 0) {
      const char **tList = (const char **)arenaAlloc(sData, sizeof(char *) * e.typeCount, alignof(char *));
      if (!tList) return loadFailed("Arena de dados esgotada (tipos)");
      for (uint8_t i = 0; i < e.typeCount; i++) {
        const char *t = internString(typesArr[i] | "normal");
        if (!t) return loadFailed("Arena de textos esgotada (tipos)");
        tList[i] = t;
      }
      e.types = tList;
    } else {
      e.typeCount = 1;
      e.types = kMissingNoTypes;
    }

    // 2. Egg Groups
    JsonArray eggArr = obj["egg_groups"].as<JsonArray>();
    e.eggGroupCount = eggArr.isNull() ? 0 : (uint8_t)eggArr.size();
    if (e.eggGroupCount > 0) {
      const char **egList = (const char **)arenaAlloc(sData, sizeof(char *) * e.eggGroupCount, alignof(char *));
      if (!egList) return loadFailed("Arena de dados esgotada (grupos de ovo)");
      for (uint8_t i = 0; i < e.eggGroupCount; i++) {
        const char *g = internString(eggArr[i] | "undiscovered");
        if (!g) return loadFailed("Arena de textos esgotada (grupos de ovo)");
        egList[i] = g;
      }
      e.eggGroups = egList;
    } else {
      e.eggGroups = nullptr;
    }

    // 3. Base Stats
    JsonObject st = obj["stats"].as<JsonObject>();
    e.stats.hp             = st["hp"] | 10;
    e.stats.attack         = st["attack"] | 10;
    e.stats.defense        = st["defense"] | 10;
    e.stats.specialAttack  = st["special-attack"] | 10;
    e.stats.specialDefense = st["special-defense"] | 10;
    e.stats.speed          = st["speed"] | 10;

    // 4. Evolutions
    JsonArray evoArr = obj["evolves_to"].as<JsonArray>();
    e.evolutionCount = evoArr.isNull() ? 0 : (uint8_t)evoArr.size();
    if (e.evolutionCount > 0) {
      NationalDexEvolution *evos = (NationalDexEvolution *)arenaAlloc(
          sData, sizeof(NationalDexEvolution) * e.evolutionCount, alignof(NationalDexEvolution));
      if (!evos) return loadFailed("Arena de dados esgotada (evolucoes)");

      for (uint8_t i = 0; i < e.evolutionCount; i++) {
        JsonObject evoObj = evoArr[i];
        evos[i].id = evoObj["id"] | 0;

        JsonArray mArr = evoObj["methods"].as<JsonArray>();
        evos[i].methodCount = mArr.isNull() ? 0 : (uint8_t)mArr.size();
        if (evos[i].methodCount > 0) {
          NationalDexEvolutionMethod *methods = (NationalDexEvolutionMethod *)arenaAlloc(
              sData, sizeof(NationalDexEvolutionMethod) * evos[i].methodCount,
              alignof(NationalDexEvolutionMethod));
          if (!methods) return loadFailed("Arena de dados esgotada (metodos)");

          for (uint8_t m = 0; m < evos[i].methodCount; m++) {
            const char *trigger = internString(mArr[m]["trigger"] | "level-up");
            if (!trigger) return loadFailed("Arena de textos esgotada (metodos)");
            methods[m].trigger = trigger;
            methods[m].minLevel = mArr[m]["min_level"] | 0;
          }
          evos[i].methods = methods;
        } else {
          evos[i].methods = nullptr;
        }
      }
      e.evolvesTo = evos;
    } else {
      e.evolvesTo = nullptr;
    }
    // Serial.printf("[NationalDex], Entrada %u processada.\n", idx);
  }

  sEntryCount = idx;
  // Serial.printf("[NationalDex] %u especies carregadas na PSRAM (%u bytes usados)\n",
  //               sEntryCount, (unsigned)(sStrings.used + sData.used + sEntryCount * sizeof(NationalDexEntry)));

  Serial.println("[NationalDex], Processamento concluído.");
  return true;
}

// =========================================================================
// CONSULTAS (SEMPRE RETORNAM MISSINGNO COMO FALLBACK)
// =========================================================================

uint16_t nationalDexCount() {
  return sEntryCount;
}

static const NationalDexEntry *findEntryOrFallback(uint16_t id, bool *found = nullptr) {
  if (found) *found = false;
  if (!sEntries || sEntryCount == 0 || id == 0) {
    return &kMissingNo;
  }

  // Busca direta O(1) caso o JSON esteja sequencial (1..N)
  if (id <= sEntryCount && sEntries[id - 1].id == id) {
    if (found) *found = true;
    return &sEntries[id - 1];
  }

  // Fallback: busca binária
  int low = 0, high = (int)sEntryCount - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;
    if (sEntries[mid].id == id) {
      if (found) *found = true;
      return &sEntries[mid];
    }
    if (sEntries[mid].id < id) low = mid + 1;
    else high = mid - 1;
  }

  return &kMissingNo;
}

const NationalDexEntry *nationalDexFind(uint16_t id) {
  return findEntryOrFallback(id);
}

bool nationalDexGetEvolvesTo(uint16_t id, const NationalDexEvolution **out, uint8_t *count) {
  bool found = false;
  const NationalDexEntry *entry = findEntryOrFallback(id, &found);
  if (out) *out = entry->evolvesTo;
  if (count) *count = entry->evolutionCount;
  return found;
}

bool nationalDexGetStats(uint16_t id, NationalDexStats *out) {
  bool found = false;
  const NationalDexEntry *entry = findEntryOrFallback(id, &found);
  if (out) *out = entry->stats;
  return found;
}

bool nationalDexGetTypes(uint16_t id, const char *const **out, uint8_t *count) {
  bool found = false;
  const NationalDexEntry *entry = findEntryOrFallback(id, &found);
  if (out) *out = entry->types;
  if (count) *count = entry->typeCount;
  return found;
}

bool nationalDexGetGrowthRate(uint16_t id, const char **out) {
  bool found = false;
  const NationalDexEntry *entry = findEntryOrFallback(id, &found);
  if (out) *out = entry->growthRate;
  return found;
}
