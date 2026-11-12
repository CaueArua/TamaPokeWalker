#pragma once
#include <stdint.h>

// GENERADO por tools/gen_dex.py desde tools/dex_data.py - no editar

#define DEX_COUNT 92
#define DEX_BITMAP_BYTES ((DEX_COUNT + 7) / 8)
#define DEX_EEVEE 34  // indice sequencial del Eevee

#define DEX_LANG_COUNT 6

// rareza: 0 = solo por evolucion, 1 = comun, 2 = raro, 3 = legendario
enum : uint8_t { R_EVO = 0, R_COMUN, R_RARO, R_LEGENDARIO };

// tipos de combate: datos actuales de las especies incluidas
enum : uint8_t {
  TYPE_NONE = 0, TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_GRASS,
  TYPE_ICE, TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_FLYING, TYPE_PSYCHIC,
  TYPE_BUG, TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON, TYPE_DARK, TYPE_STEEL, TYPE_FAIRY
};

enum EvolutionCondition : uint8_t {
  EVO_LEVEL = 0, EVO_BOND, EVO_DAY_BOND, EVO_NIGHT_BOND,
  EVO_ATK_GT_DEF, EVO_DEF_GT_ATK, EVO_ATK_EQ_DEF
};
struct EvolutionRule {
  uint16_t from;
  uint16_t to;
  uint8_t minLevel;
  uint8_t condition;
};

struct DexEntry {
  const char *name;
  uint16_t evolvesTo;   // nuevo indice de dex, 0 = forma final
  uint8_t evolveLevel;
  uint8_t rarity;       // sale de huevo si > 0
  uint16_t accent;      // color RGB565 del tipo para la UI
  uint8_t bHp, bAtk, bDef, bSpe;  // base stats reales
  uint8_t type1, type2; // tipos de combate, TYPE_NONE si no hay secundario
  uint8_t biome;        // 0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve
  uint16_t natNum;      // numero oficial de la National Dex
};

static const DexEntry DEX_TBL[DEX_COUNT + 1] = {
  { "?", 0, 0, 0, 0x2946, 50, 50, 50, 50, TYPE_NONE, TYPE_NONE, 0, 0 },  // 0: sin usar
  { "BULBASAUR", 2, 16, R_COMUN, 0x3C49, 45, 49, 49, 45, TYPE_GRASS, TYPE_POISON, 2, 1 },  // idx 1 -> nat #1 grass/poison
  { "IVYSAUR", 3, 32, R_EVO, 0x3C49, 60, 62, 63, 60, TYPE_GRASS, TYPE_POISON, 2, 2 },  // idx 2 -> nat #2 grass/poison
  { "VENUSAUR", 0, 0, R_EVO, 0x3C49, 80, 82, 83, 80, TYPE_GRASS, TYPE_POISON, 2, 3 },  // idx 3 -> nat #3 grass/poison
  { "CHARMANDER", 5, 16, R_COMUN, 0xEA87, 39, 52, 43, 65, TYPE_FIRE, TYPE_NONE, 3, 4 },  // idx 4 -> nat #4 fire
  { "CHARMELEON", 6, 36, R_EVO, 0xEA87, 58, 64, 58, 80, TYPE_FIRE, TYPE_NONE, 3, 5 },  // idx 5 -> nat #5 fire
  { "CHARIZARD", 0, 0, R_EVO, 0xEA87, 78, 84, 78, 100, TYPE_FIRE, TYPE_FLYING, 3, 6 },  // idx 6 -> nat #6 fire/flying
  { "SQUIRTLE", 8, 16, R_COMUN, 0x4C98, 44, 48, 65, 43, TYPE_WATER, TYPE_NONE, 1, 7 },  // idx 7 -> nat #7 water
  { "WARTORTLE", 9, 36, R_EVO, 0x4C98, 59, 63, 80, 58, TYPE_WATER, TYPE_NONE, 1, 8 },  // idx 8 -> nat #8 water
  { "BLASTOISE", 0, 0, R_EVO, 0x4C98, 79, 83, 100, 78, TYPE_WATER, TYPE_NONE, 1, 9 },  // idx 9 -> nat #9 water
  { "PIKACHU", 11, 30, R_EVO, 0xBCA1, 35, 55, 40, 90, TYPE_ELECTRIC, TYPE_NONE, 0, 25 },  // idx 10 -> nat #25 electric
  { "RAICHU", 0, 0, R_EVO, 0xBCA1, 60, 90, 55, 110, TYPE_ELECTRIC, TYPE_NONE, 0, 26 },  // idx 11 -> nat #26 electric
  { "NIDORAN H", 13, 16, R_COMUN, 0x8A73, 55, 47, 52, 41, TYPE_POISON, TYPE_NONE, 0, 29 },  // idx 12 -> nat #29 poison
  { "NIDORINA", 14, 30, R_EVO, 0x8A73, 70, 62, 67, 56, TYPE_POISON, TYPE_NONE, 0, 30 },  // idx 13 -> nat #30 poison
  { "NIDOQUEEN", 0, 0, R_EVO, 0x8A73, 90, 92, 87, 76, TYPE_POISON, TYPE_GROUND, 0, 31 },  // idx 14 -> nat #31 poison/ground
  { "NIDORAN M", 16, 16, R_COMUN, 0x8A73, 46, 57, 40, 50, TYPE_POISON, TYPE_NONE, 0, 32 },  // idx 15 -> nat #32 poison
  { "NIDORINO", 17, 30, R_EVO, 0x8A73, 61, 72, 57, 65, TYPE_POISON, TYPE_NONE, 0, 33 },  // idx 16 -> nat #33 poison
  { "NIDOKING", 0, 0, R_EVO, 0x8A73, 81, 102, 77, 85, TYPE_POISON, TYPE_GROUND, 0, 34 },  // idx 17 -> nat #34 poison/ground
  { "VULPIX", 19, 30, R_COMUN, 0xEA87, 38, 41, 40, 65, TYPE_FIRE, TYPE_NONE, 3, 37 },  // idx 18 -> nat #37 fire
  { "NINETALES", 0, 0, R_EVO, 0xEA87, 73, 76, 75, 100, TYPE_FIRE, TYPE_NONE, 3, 38 },  // idx 19 -> nat #38 fire
  { "GROWLITHE", 21, 30, R_RARO, 0xEA87, 55, 70, 45, 60, TYPE_FIRE, TYPE_NONE, 3, 58 },  // idx 20 -> nat #58 fire
  { "ARCANINE", 0, 0, R_EVO, 0xEA87, 90, 110, 80, 95, TYPE_FIRE, TYPE_NONE, 3, 59 },  // idx 21 -> nat #59 fire
  { "ABRA", 23, 16, R_COMUN, 0xD28F, 25, 20, 15, 90, TYPE_PSYCHIC, TYPE_NONE, 0, 63 },  // idx 22 -> nat #63 psychic
  { "KADABRA", 24, 40, R_EVO, 0xD28F, 40, 35, 30, 105, TYPE_PSYCHIC, TYPE_NONE, 0, 64 },  // idx 23 -> nat #64 psychic
  { "ALAKAZAM", 0, 0, R_EVO, 0xD28F, 55, 50, 45, 120, TYPE_PSYCHIC, TYPE_NONE, 0, 65 },  // idx 24 -> nat #65 psychic
  { "GEODUDE", 26, 25, R_COMUN, 0x9407, 40, 80, 100, 20, TYPE_ROCK, TYPE_GROUND, 4, 74 },  // idx 25 -> nat #74 rock/ground
  { "GRAVELER", 27, 40, R_EVO, 0x9407, 55, 95, 115, 35, TYPE_ROCK, TYPE_GROUND, 4, 75 },  // idx 26 -> nat #75 rock/ground
  { "GOLEM", 0, 0, R_EVO, 0x9407, 80, 120, 130, 45, TYPE_ROCK, TYPE_GROUND, 4, 76 },  // idx 27 -> nat #76 rock/ground
  { "GASTLY", 29, 25, R_COMUN, 0x6AD3, 30, 35, 30, 80, TYPE_GHOST, TYPE_POISON, 0, 92 },  // idx 28 -> nat #92 ghost/poison
  { "HAUNTER", 30, 40, R_EVO, 0x6AD3, 45, 50, 45, 95, TYPE_GHOST, TYPE_POISON, 0, 93 },  // idx 29 -> nat #93 ghost/poison
  { "GENGAR", 0, 0, R_EVO, 0x6AD3, 60, 65, 60, 110, TYPE_GHOST, TYPE_POISON, 0, 94 },  // idx 30 -> nat #94 ghost/poison
  { "CHANSEY", 0, 0, R_RARO, 0x8C4D, 250, 5, 5, 50, TYPE_NORMAL, TYPE_NONE, 0, 113 },  // idx 31 -> nat #113 normal
  { "SCYTHER", 0, 0, R_RARO, 0x7CC4, 70, 110, 80, 105, TYPE_BUG, TYPE_FLYING, 2, 123 },  // idx 32 -> nat #123 bug/flying
  { "LAPRAS", 0, 0, R_RARO, 0x4C98, 130, 85, 80, 60, TYPE_WATER, TYPE_ICE, 1, 131 },  // idx 33 -> nat #131 water/ice
  { "EEVEE", 35, 30, R_COMUN, 0x8C4D, 55, 55, 50, 55, TYPE_NORMAL, TYPE_NONE, 0, 133 },  // idx 34 -> nat #133 normal
  { "VAPOREON", 0, 0, R_EVO, 0x4C98, 130, 65, 60, 65, TYPE_WATER, TYPE_NONE, 1, 134 },  // idx 35 -> nat #134 water
  { "JOLTEON", 0, 0, R_EVO, 0xBCA1, 65, 65, 60, 130, TYPE_ELECTRIC, TYPE_NONE, 0, 135 },  // idx 36 -> nat #135 electric
  { "FLAREON", 0, 0, R_EVO, 0xEA87, 65, 130, 60, 65, TYPE_FIRE, TYPE_NONE, 3, 136 },  // idx 37 -> nat #136 fire
  { "AERODACTYL", 0, 0, R_RARO, 0x9407, 80, 105, 65, 130, TYPE_ROCK, TYPE_FLYING, 4, 142 },  // idx 38 -> nat #142 rock/flying
  { "SNORLAX", 0, 0, R_RARO, 0x8C4D, 160, 110, 65, 30, TYPE_NORMAL, TYPE_NONE, 0, 143 },  // idx 39 -> nat #143 normal
  { "ZAPDOS", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 90, 85, 100, TYPE_ELECTRIC, TYPE_FLYING, 0, 145 },  // idx 40 -> nat #145 electric/flying
  { "DRATINI", 42, 30, R_RARO, 0x5A98, 41, 64, 45, 50, TYPE_DRAGON, TYPE_NONE, 1, 147 },  // idx 41 -> nat #147 dragon
  { "DRAGONAIR", 43, 55, R_EVO, 0x5A98, 61, 84, 65, 70, TYPE_DRAGON, TYPE_NONE, 1, 148 },  // idx 42 -> nat #148 dragon
  { "DRAGONITE", 0, 0, R_EVO, 0x5A98, 91, 134, 95, 80, TYPE_DRAGON, TYPE_FLYING, 1, 149 },  // idx 43 -> nat #149 dragon/flying
  { "MEWTWO", 0, 0, R_LEGENDARIO, 0xD28F, 106, 110, 90, 130, TYPE_PSYCHIC, TYPE_NONE, 0, 150 },  // idx 44 -> nat #150 psychic
  { "MEW", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, TYPE_PSYCHIC, TYPE_NONE, 0, 151 },  // idx 45 -> nat #151 psychic
  { "CHIKORITA", 47, 16, R_COMUN, 0x3C49, 45, 49, 65, 45, TYPE_GRASS, TYPE_NONE, 2, 152 },  // idx 46 -> nat #152 grass
  { "BAYLEEF", 48, 32, R_EVO, 0x3C49, 60, 62, 80, 60, TYPE_GRASS, TYPE_NONE, 2, 153 },  // idx 47 -> nat #153 grass
  { "MEGANIUM", 0, 0, R_EVO, 0x3C49, 80, 82, 100, 80, TYPE_GRASS, TYPE_NONE, 2, 154 },  // idx 48 -> nat #154 grass
  { "CYNDAQUIL", 50, 14, R_COMUN, 0xEA87, 39, 52, 43, 65, TYPE_FIRE, TYPE_NONE, 3, 155 },  // idx 49 -> nat #155 fire
  { "QUILAVA", 51, 36, R_EVO, 0xEA87, 58, 64, 58, 80, TYPE_FIRE, TYPE_NONE, 3, 156 },  // idx 50 -> nat #156 fire
  { "TYPHLOSION", 0, 0, R_EVO, 0xEA87, 78, 84, 78, 100, TYPE_FIRE, TYPE_NONE, 3, 157 },  // idx 51 -> nat #157 fire
  { "TOTODILE", 53, 18, R_COMUN, 0x4C98, 50, 65, 64, 43, TYPE_WATER, TYPE_NONE, 1, 158 },  // idx 52 -> nat #158 water
  { "CROCONAW", 54, 30, R_EVO, 0x4C98, 65, 80, 80, 58, TYPE_WATER, TYPE_NONE, 1, 159 },  // idx 53 -> nat #159 water
  { "FERALIGATR", 0, 0, R_EVO, 0x4C98, 85, 105, 100, 78, TYPE_WATER, TYPE_NONE, 1, 160 },  // idx 54 -> nat #160 water
  { "PICHU", 10, 10, R_COMUN, 0xBCA1, 20, 40, 15, 60, TYPE_ELECTRIC, TYPE_NONE, 0, 172 },  // idx 55 -> nat #172 electric
  { "ESPEON", 0, 0, R_EVO, 0xD28F, 65, 65, 60, 110, TYPE_PSYCHIC, TYPE_NONE, 0, 196 },  // idx 56 -> nat #196 psychic
  { "UMBREON", 0, 0, R_EVO, 0x5A6E, 95, 65, 110, 65, TYPE_DARK, TYPE_NONE, 0, 197 },  // idx 57 -> nat #197 dark
  { "SCIZOR", 0, 0, R_EVO, 0x7CC4, 70, 130, 100, 65, TYPE_BUG, TYPE_STEEL, 2, 212 },  // idx 58 -> nat #212 bug/steel
  { "BLISSEY", 0, 0, R_EVO, 0x8C4D, 255, 10, 10, 55, TYPE_NORMAL, TYPE_NONE, 0, 242 },  // idx 59 -> nat #242 normal
  { "RAIKOU", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 85, 75, 115, TYPE_ELECTRIC, TYPE_NONE, 0, 243 },  // idx 60 -> nat #243 electric
  { "ENTEI", 0, 0, R_LEGENDARIO, 0xEA87, 115, 115, 85, 100, TYPE_FIRE, TYPE_NONE, 3, 244 },  // idx 61 -> nat #244 fire
  { "SUICUNE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 75, 115, 85, TYPE_WATER, TYPE_NONE, 1, 245 },  // idx 62 -> nat #245 water
  { "LARVITAR", 64, 30, R_RARO, 0x9407, 50, 64, 50, 41, TYPE_ROCK, TYPE_GROUND, 4, 246 },  // idx 63 -> nat #246 rock/ground
  { "PUPITAR", 65, 55, R_EVO, 0x9407, 70, 84, 70, 51, TYPE_ROCK, TYPE_GROUND, 4, 247 },  // idx 64 -> nat #247 rock/ground
  { "TYRANITAR", 0, 0, R_EVO, 0x9407, 100, 134, 110, 61, TYPE_ROCK, TYPE_DARK, 4, 248 },  // idx 65 -> nat #248 rock/dark
  { "LUGIA", 0, 0, R_LEGENDARIO, 0xD28F, 106, 90, 130, 110, TYPE_PSYCHIC, TYPE_FLYING, 0, 249 },  // idx 66 -> nat #249 psychic/flying
  { "CELEBI", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, TYPE_PSYCHIC, TYPE_GRASS, 0, 251 },  // idx 67 -> nat #251 psychic/grass
  { "TREECKO", 69, 16, R_COMUN, 0x3C49, 40, 45, 35, 70, TYPE_GRASS, TYPE_NONE, 2, 252 },  // idx 68 -> nat #252 grass
  { "GROVYLE", 70, 36, R_EVO, 0x3C49, 50, 65, 45, 95, TYPE_GRASS, TYPE_NONE, 2, 253 },  // idx 69 -> nat #253 grass
  { "SCEPTILE", 0, 0, R_EVO, 0x3C49, 70, 85, 65, 120, TYPE_GRASS, TYPE_NONE, 2, 254 },  // idx 70 -> nat #254 grass
  { "TORCHIC", 72, 16, R_COMUN, 0xEA87, 45, 60, 40, 45, TYPE_FIRE, TYPE_NONE, 3, 255 },  // idx 71 -> nat #255 fire
  { "COMBUSKEN", 73, 36, R_EVO, 0xEA87, 60, 85, 60, 55, TYPE_FIRE, TYPE_FIGHTING, 3, 256 },  // idx 72 -> nat #256 fire/fighting
  { "BLAZIKEN", 0, 0, R_EVO, 0xEA87, 80, 120, 70, 80, TYPE_FIRE, TYPE_FIGHTING, 3, 257 },  // idx 73 -> nat #257 fire/fighting
  { "MUDKIP", 75, 16, R_COMUN, 0x4C98, 50, 70, 50, 40, TYPE_WATER, TYPE_NONE, 1, 258 },  // idx 74 -> nat #258 water
  { "MARSHTOMP", 76, 36, R_EVO, 0x4C98, 70, 85, 70, 50, TYPE_WATER, TYPE_GROUND, 1, 259 },  // idx 75 -> nat #259 water/ground
  { "SWAMPERT", 0, 0, R_EVO, 0x4C98, 100, 110, 90, 60, TYPE_WATER, TYPE_GROUND, 1, 260 },  // idx 76 -> nat #260 water/ground
  { "RALTS", 78, 20, R_COMUN, 0xD28F, 28, 25, 25, 40, TYPE_PSYCHIC, TYPE_FAIRY, 0, 280 },  // idx 77 -> nat #280 psychic/fairy
  { "KIRLIA", 79, 30, R_EVO, 0xD28F, 38, 35, 35, 50, TYPE_PSYCHIC, TYPE_FAIRY, 0, 281 },  // idx 78 -> nat #281 psychic/fairy
  { "GARDEVOIR", 0, 0, R_EVO, 0xD28F, 68, 65, 65, 80, TYPE_PSYCHIC, TYPE_FAIRY, 0, 282 },  // idx 79 -> nat #282 psychic/fairy
  { "BAGON", 81, 30, R_RARO, 0x5A98, 45, 75, 60, 50, TYPE_DRAGON, TYPE_NONE, 1, 371 },  // idx 80 -> nat #371 dragon
  { "SHELGON", 82, 50, R_EVO, 0x5A98, 65, 95, 100, 50, TYPE_DRAGON, TYPE_NONE, 1, 372 },  // idx 81 -> nat #372 dragon
  { "SALAMENCE", 0, 0, R_EVO, 0x5A98, 95, 135, 80, 100, TYPE_DRAGON, TYPE_FLYING, 1, 373 },  // idx 82 -> nat #373 dragon/flying
  { "BELDUM", 84, 20, R_RARO, 0x7C73, 40, 55, 80, 30, TYPE_STEEL, TYPE_PSYCHIC, 4, 374 },  // idx 83 -> nat #374 steel/psychic
  { "METANG", 85, 45, R_EVO, 0x7C73, 60, 75, 100, 50, TYPE_STEEL, TYPE_PSYCHIC, 4, 375 },  // idx 84 -> nat #375 steel/psychic
  { "METAGROSS", 0, 0, R_EVO, 0x7C73, 80, 135, 130, 70, TYPE_STEEL, TYPE_PSYCHIC, 4, 376 },  // idx 85 -> nat #376 steel/psychic
  { "REGIROCK", 0, 0, R_LEGENDARIO, 0x9407, 80, 100, 200, 50, TYPE_ROCK, TYPE_NONE, 4, 377 },  // idx 86 -> nat #377 rock
  { "REGICE", 0, 0, R_LEGENDARIO, 0x4DB8, 80, 50, 100, 50, TYPE_ICE, TYPE_NONE, 5, 378 },  // idx 87 -> nat #378 ice
  { "REGISTEEL", 0, 0, R_LEGENDARIO, 0x7C73, 80, 75, 150, 50, TYPE_STEEL, TYPE_NONE, 4, 379 },  // idx 88 -> nat #379 steel
  { "LATIAS", 0, 0, R_LEGENDARIO, 0x5A98, 80, 80, 90, 110, TYPE_DRAGON, TYPE_PSYCHIC, 1, 380 },  // idx 89 -> nat #380 dragon/psychic
  { "KYOGRE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 100, 90, 90, TYPE_WATER, TYPE_NONE, 1, 382 },  // idx 90 -> nat #382 water
  { "JIRACHI", 0, 0, R_LEGENDARIO, 0x7C73, 100, 100, 100, 100, TYPE_STEEL, TYPE_PSYCHIC, 4, 385 },  // idx 91 -> nat #385 steel/psychic
  { "DEOXYS", 0, 0, R_LEGENDARIO, 0xD28F, 50, 150, 50, 150, TYPE_PSYCHIC, TYPE_NONE, 0, 386 },  // idx 92 -> nat #386 psychic
};

// Evoluciones, incluidas ramas y condiciones especiales.
static const EvolutionRule EVOLUTION_RULES[] = {
  { 1, 2, 16, EVO_LEVEL },
  { 2, 3, 32, EVO_LEVEL },
  { 4, 5, 16, EVO_LEVEL },
  { 5, 6, 36, EVO_LEVEL },
  { 7, 8, 16, EVO_LEVEL },
  { 8, 9, 36, EVO_LEVEL },
  { 10, 11, 30, EVO_LEVEL },
  { 12, 13, 16, EVO_LEVEL },
  { 13, 14, 30, EVO_LEVEL },
  { 15, 16, 16, EVO_LEVEL },
  { 16, 17, 30, EVO_LEVEL },
  { 18, 19, 30, EVO_LEVEL },
  { 20, 21, 30, EVO_LEVEL },
  { 22, 23, 16, EVO_LEVEL },
  { 23, 24, 40, EVO_LEVEL },
  { 25, 26, 25, EVO_LEVEL },
  { 26, 27, 40, EVO_LEVEL },
  { 28, 29, 25, EVO_LEVEL },
  { 29, 30, 40, EVO_LEVEL },
  { 34, 35, 30, EVO_LEVEL },
  { 41, 42, 30, EVO_LEVEL },
  { 42, 43, 55, EVO_LEVEL },
  { 46, 47, 16, EVO_LEVEL },
  { 47, 48, 32, EVO_LEVEL },
  { 49, 50, 14, EVO_LEVEL },
  { 50, 51, 36, EVO_LEVEL },
  { 52, 53, 18, EVO_LEVEL },
  { 53, 54, 30, EVO_LEVEL },
  { 55, 10, 10, EVO_BOND },
  { 63, 64, 30, EVO_LEVEL },
  { 64, 65, 55, EVO_LEVEL },
  { 68, 69, 16, EVO_LEVEL },
  { 69, 70, 36, EVO_LEVEL },
  { 71, 72, 16, EVO_LEVEL },
  { 72, 73, 36, EVO_LEVEL },
  { 74, 75, 16, EVO_LEVEL },
  { 75, 76, 36, EVO_LEVEL },
  { 77, 78, 20, EVO_LEVEL },
  { 78, 79, 30, EVO_LEVEL },
  { 80, 81, 30, EVO_LEVEL },
  { 81, 82, 50, EVO_LEVEL },
  { 83, 84, 20, EVO_LEVEL },
  { 84, 85, 45, EVO_LEVEL },
  { 31, 59, 30, EVO_BOND },
  { 32, 58, 30, EVO_LEVEL },
  { 34, 36, 30, EVO_LEVEL },
  { 34, 37, 30, EVO_LEVEL },
  { 34, 56, 30, EVO_DAY_BOND },
  { 34, 57, 30, EVO_NIGHT_BOND },
};
#define EVOLUTION_RULE_COUNT 49

// nombres localizados en el orden de Lang: ES, EN, FR, DE, IT, PT
static const char *const DEX_NAMES[DEX_LANG_COUNT][DEX_COUNT + 1] = {
  // ES
  { "?", "BULBASAUR", "IVYSAUR", "VENUSAUR", "CHARMANDER", "CHARMELEON", "CHARIZARD", "SQUIRTLE", "WARTORTLE", "BLASTOISE", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "VULPIX", "NINETALES", "GROWLITHE", "ARCANINE", "ABRA", "KADABRA", "ALAKAZAM", "GEODUDE", "GRAVELER", "GOLEM", "GASTLY", "HAUNTER", "GENGAR", "CHANSEY", "SCYTHER", "LAPRAS", "EEVEE", "VAPOREON", "JOLTEON", "FLAREON", "AERODACTYL", "SNORLAX", "ZAPDOS", "DRATINI", "DRAGONAIR", "DRAGONITE", "MEWTWO", "MEW", "CHIKORITA", "BAYLEEF", "MEGANIUM", "CYNDAQUIL", "QUILAVA", "TYPHLOSION", "TOTODILE", "CROCONAW", "FERALIGATR", "PICHU", "ESPEON", "UMBREON", "SCIZOR", "BLISSEY", "RAIKOU", "ENTEI", "SUICUNE", "LARVITAR", "PUPITAR", "TYRANITAR", "LUGIA", "CELEBI", "TREECKO", "GROVYLE", "SCEPTILE", "TORCHIC", "COMBUSKEN", "BLAZIKEN", "MUDKIP", "MARSHTOMP", "SWAMPERT", "RALTS", "KIRLIA", "GARDEVOIR", "BAGON", "SHELGON", "SALAMENCE", "BELDUM", "METANG", "METAGROSS", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
  // EN
  { "?", "BULBASAUR", "IVYSAUR", "VENUSAUR", "CHARMANDER", "CHARMELEON", "CHARIZARD", "SQUIRTLE", "WARTORTLE", "BLASTOISE", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "VULPIX", "NINETALES", "GROWLITHE", "ARCANINE", "ABRA", "KADABRA", "ALAKAZAM", "GEODUDE", "GRAVELER", "GOLEM", "GASTLY", "HAUNTER", "GENGAR", "CHANSEY", "SCYTHER", "LAPRAS", "EEVEE", "VAPOREON", "JOLTEON", "FLAREON", "AERODACTYL", "SNORLAX", "ZAPDOS", "DRATINI", "DRAGONAIR", "DRAGONITE", "MEWTWO", "MEW", "CHIKORITA", "BAYLEEF", "MEGANIUM", "CYNDAQUIL", "QUILAVA", "TYPHLOSION", "TOTODILE", "CROCONAW", "FERALIGATR", "PICHU", "ESPEON", "UMBREON", "SCIZOR", "BLISSEY", "RAIKOU", "ENTEI", "SUICUNE", "LARVITAR", "PUPITAR", "TYRANITAR", "LUGIA", "CELEBI", "TREECKO", "GROVYLE", "SCEPTILE", "TORCHIC", "COMBUSKEN", "BLAZIKEN", "MUDKIP", "MARSHTOMP", "SWAMPERT", "RALTS", "KIRLIA", "GARDEVOIR", "BAGON", "SHELGON", "SALAMENCE", "BELDUM", "METANG", "METAGROSS", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
  // FR
  { "?", "BULBIZARRE", "HERBIZARRE", "FLORIZARRE", "SALAMECHE", "REPTINCEL", "DRACAUFEU", "CARAPUCE", "CARABAFFE", "TORTANK", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "GOUPIX", "FEUNARD", "CANINOS", "ARCANIN", "ABRA", "KADABRA", "ALAKAZAM", "RACAILLOU", "GRAVALANCH", "GROLEM", "FANTOMINUS", "SPECTRUM", "ECTOPLASMA", "LEVEINARD", "INSECATEUR", "LOKHLASS", "EVOLI", "AQUALI", "VOLTALI", "PYROLI", "PTERA", "RONFLEX", "ELECTHOR", "MINIDRACO", "DRACO", "DRACOLOSSE", "MEWTWO", "MEW", "GERMIGNON", "MACRONIUM", "MEGANIUM", "HERICENDRE", "FEURISSON", "TYPHLOSION", "KAIMINUS", "CROCRODIL", "ALIGATUEUR", "PICHU", "MENTALI", "NOCTALI", "CIZAYOX", "LEUPHORIE", "RAIKOU", "ENTEI", "SUICUNE", "EMBRYLEX", "YMPHECT", "TYRANOCIF", "LUGIA", "CELEBI", "ARCKO", "MASSKO", "JUNGKO", "POUSSIFEU", "GALIFEU", "BRASEGALI", "GOBOU", "FLOBIO", "LAGGRON", "TARSAL", "KIRLIA", "GARDEVOIR", "DRABY", "DRACKHAUS", "DRATTAK", "TERHAL", "METANG", "METALOSSE", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
  // DE
  { "?", "BISASAM", "BISAKNOSP", "BISAFLOR", "GLUMANDA", "GLUTEXO", "GLURAK", "SCHIGGY", "SCHILLOK", "TURTOK", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "VULPIX", "VULNONA", "FUKANO", "ARKANI", "ABRA", "KADABRA", "SIMSALA", "KLEINSTEIN", "GEOROK", "GEOWAZ", "NEBULAK", "ALPOLLO", "GENGAR", "CHANEIRA", "SICHLOR", "LAPRAS", "EVOLI", "AQUANA", "BLITZA", "FLAMARA", "AERODACTYL", "RELAXO", "ZAPDOS", "DRATINI", "DRAGONIR", "DRAGORAN", "MEWTU", "MEW", "ENDIVIE", "LORBLATT", "MEGANIE", "FEURIGEL", "IGELAVAR", "TORNUPTO", "KARNIMANI", "TYRACROC", "IMPERGATOR", "PICHU", "PSIANA", "NACHTARA", "SCHEROX", "HEITEIRA", "RAIKOU", "ENTEI", "SUICUNE", "LARVITAR", "PUPITAR", "DESPOTAR", "LUGIA", "CELEBI", "GECKARBOR", "REPTAIN", "GEWALDRO", "FLEMMLI", "JUNGGLUT", "LOHGOCK", "HYDROPI", "MOORABBEL", "SUMPEX", "TRASLA", "KIRLIA", "GUARDEVOIR", "KINDWURM", "DRASCHEL", "BRUTALANDA", "TANHEL", "METANG", "METAGROSS", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
  // IT
  { "?", "BULBASAUR", "IVYSAUR", "VENUSAUR", "CHARMANDER", "CHARMELEON", "CHARIZARD", "SQUIRTLE", "WARTORTLE", "BLASTOISE", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "VULPIX", "NINETALES", "GROWLITHE", "ARCANINE", "ABRA", "KADABRA", "ALAKAZAM", "GEODUDE", "GRAVELER", "GOLEM", "GASTLY", "HAUNTER", "GENGAR", "CHANSEY", "SCYTHER", "LAPRAS", "EEVEE", "VAPOREON", "JOLTEON", "FLAREON", "AERODACTYL", "SNORLAX", "ZAPDOS", "DRATINI", "DRAGONAIR", "DRAGONITE", "MEWTWO", "MEW", "CHIKORITA", "BAYLEEF", "MEGANIUM", "CYNDAQUIL", "QUILAVA", "TYPHLOSION", "TOTODILE", "CROCONAW", "FERALIGATR", "PICHU", "ESPEON", "UMBREON", "SCIZOR", "BLISSEY", "RAIKOU", "ENTEI", "SUICUNE", "LARVITAR", "PUPITAR", "TYRANITAR", "LUGIA", "CELEBI", "TREECKO", "GROVYLE", "SCEPTILE", "TORCHIC", "COMBUSKEN", "BLAZIKEN", "MUDKIP", "MARSHTOMP", "SWAMPERT", "RALTS", "KIRLIA", "GARDEVOIR", "BAGON", "SHELGON", "SALAMENCE", "BELDUM", "METANG", "METAGROSS", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
  // PT
  { "?", "BULBASAUR", "IVYSAUR", "VENUSAUR", "CHARMANDER", "CHARMELEON", "CHARIZARD", "SQUIRTLE", "WARTORTLE", "BLASTOISE", "PIKACHU", "RAICHU", "NIDORAN F", "NIDORINA", "NIDOQUEEN", "NIDORAN M", "NIDORINO", "NIDOKING", "VULPIX", "NINETALES", "GROWLITHE", "ARCANINE", "ABRA", "KADABRA", "ALAKAZAM", "GEODUDE", "GRAVELER", "GOLEM", "GASTLY", "HAUNTER", "GENGAR", "CHANSEY", "SCYTHER", "LAPRAS", "EEVEE", "VAPOREON", "JOLTEON", "FLAREON", "AERODACTYL", "SNORLAX", "ZAPDOS", "DRATINI", "DRAGONAIR", "DRAGONITE", "MEWTWO", "MEW", "CHIKORITA", "BAYLEEF", "MEGANIUM", "CYNDAQUIL", "QUILAVA", "TYPHLOSION", "TOTODILE", "CROCONAW", "FERALIGATR", "PICHU", "ESPEON", "UMBREON", "SCIZOR", "BLISSEY", "RAIKOU", "ENTEI", "SUICUNE", "LARVITAR", "PUPITAR", "TYRANITAR", "LUGIA", "CELEBI", "TREECKO", "GROVYLE", "SCEPTILE", "TORCHIC", "COMBUSKEN", "BLAZIKEN", "MUDKIP", "MARSHTOMP", "SWAMPERT", "RALTS", "KIRLIA", "GARDEVOIR", "BAGON", "SHELGON", "SALAMENCE", "BELDUM", "METANG", "METAGROSS", "REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "KYOGRE", "JIRACHI", "DEOXYS" },
};

// el primer huevo de la partida: iniciales clasicos
static const int16_t CLASSIC_DEX[] = { 1, 4, 7, 10, 34 };
#define NUM_CLASSIC_DEX 5
