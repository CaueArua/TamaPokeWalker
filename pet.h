#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "time_utils.h"
#include "dex.h"
#include "pet_growth.h"

// One game tick is one in-game minute. Lower this value to accelerate testing.
#define PET_TICK_MS 60000UL
// In-game minutes per level. Lower this value to accelerate evolution testing.
#define MINUTES_PER_LEVEL 60
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
#define EVOLVE_ANIM_MS 5200UL              // Duration of the evolution animation.
#define CEREMONY_MS 10000UL                // Duration of an on-screen farewell ceremony.
#define FAREWELL_AGE_MIN (3UL * 24 * 60)   // Minimum age for a final-form farewell.
#define RUNAWAY_TICKS 60                   // Neglect ticks required before a runaway.

/** Pet lifecycle ceremonies. */
enum : uint8_t { CER_NONE = 0, CER_FAREWELL, CER_RUNAWAY, CER_RELEASE };

/** Current presentation mood of the pet. */
enum PetMood : uint8_t { MOOD_HAPPY, MOOD_SAD, MOOD_EATING, MOOD_SLEEPING };
/** Random interaction events that can occur on the main screen. */
enum PetEventType : uint8_t { PET_EVENT_BERRY = 0, PET_EVENT_HEART, PET_EVENT_SPARKLE };
/** Personality profiles assigned to pets. */
enum PetPersonality : uint8_t {
  PERS_BALANCED = 0,
  PERS_PLAYFUL,
  PERS_BRAVE,
  PERS_CALM,
  PERS_LAZY,
};

/** Effects granted by a pet interaction, represented as a bitmask. */
enum PetInteractResult : uint8_t {
  PET_INTERACT_NONE = 0,
  PET_INTERACT_JOY = 1 << 0,
  PET_INTERACT_BOND = 1 << 1,
  PET_INTERACT_ENERGY = 1 << 2,
};

/** Daily goal categories tracked by the pet. */
enum DailyGoalType : uint8_t {
  DAILY_GOAL_CARE = 0,
  DAILY_GOAL_PLAY,
  DAILY_GOAL_BATTLE,
  DAILY_GOAL_CATCH,
  DAILY_GOAL_MEMO,
};
#define DAILY_GOAL_COUNT 3

/** Items that can be found or used during expeditions. */
enum ExpeditionItem : uint8_t {
  EXP_ITEM_SNACK = 0,
  EXP_ITEM_ENERGY,
  EXP_ITEM_CARE,
  EXP_ITEM_TRAIN,
  EXP_ITEM_NONE = 0xFF,
};
#define EXPEDITION_SITE_COUNT 3
/** Destinations available for expeditions. */
enum ExpeditionSite : uint8_t {
  EXP_SITE_COAST = 0,
  EXP_SITE_VOLCANO,
  EXP_SITE_FOREST,
  EXP_SITE_NONE = 0xFF,
};
#define EXP_ITEM_COUNT 4
#define EXP_ITEM_MAX 3

/** Transient step-reward events shown to the player. */
enum StepRewardEvent : uint8_t {
  STEP_REWARD_NONE = 0,
  STEP_REWARD_SNACK,
  STEP_REWARD_ENERGY,
  STEP_REWARD_TRAIN,
  STEP_REWARD_TRAIL_RANK,
};
#define STEP_DAILY_GOAL_COUNT 3

/** States displayed by the main-screen expedition HUD. */
enum ExpeditionHudState : uint8_t {
  EXP_HUD_HIDDEN = 0,
  EXP_HUD_ACTIVE,
  EXP_HUD_READY,
  EXP_HUD_BAG,
};

/** Battle stat that can receive an expedition training reward. */
enum TrainingStat : int8_t {
  TRAIN_STAT_ATK = 0,
  TRAIN_STAT_DEF,
  TRAIN_STAT_SPE,
};

/** Pet medal bits. */
enum : uint16_t {
  MED_LV10 = 1 << 0, MED_LV25 = 1 << 1, MED_LV50 = 1 << 2,
  MED_BERRY = 1 << 3, MED_STREAK7 = 1 << 4, MED_BOND = 1 << 5,
  MED_FINAL = 1 << 6, MED_FIT = 1 << 7, MED_HOENN_EXPLORER = 1 << 8,
};
#define MED_COUNT 9

/** Battle stat that can receive a post-battle reward. */
enum BattleRewardStat : uint8_t {
  BATTLE_REWARD_NONE = 0,
  BATTLE_REWARD_ATK,
  BATTLE_REWARD_DEF,
  BATTLE_REWARD_SPE,
};

struct BattleReward {
  BattleRewardStat stat = BATTLE_REWARD_NONE;
  uint8_t amount = 0;
};

#define PARTY_SIZE 6

/** Stat indices shared by ivs[] and evs[]. */
enum StatIdx : uint8_t { STAT_HP = 0, STAT_ATK, STAT_DEF, STAT_SPA, STAT_SPD, STAT_SPE, STAT_COUNT };
#define NATURE_COUNT 25
#define NATURE_HARDY 8
#define EV_STAT_MAX 252
#define EV_TOTAL_MAX 510

/** Returns the display name of a nature index (0..NATURE_COUNT-1). */
const char *natureName(uint8_t nature);
/** Returns +1 for a stat boosted by the nature, -1 for a lowered stat, or 0. */
int8_t natureStatMod(uint8_t nature, uint8_t statIdx);

/** Inputs and result of one stat calculation, for diagnostics. */
struct StatBreakdown {
  uint8_t base;
  uint8_t iv;
  uint8_t ev;
  uint8_t level;
  int8_t natureMod;
  uint16_t finalValue;
};

/** Highest experience value a pet can hold. */
constexpr uint32_t PET_XP_MAX = 1640000UL;

/** Persistent state stored for one party member. */
struct PartySlot {
  int16_t speciesId = 0; // Zero means empty; 1..DEX_COUNT are species IDs.
  bool shiny = false;
  char nick[12] = "";
  uint32_t ageMinutes = 0; // Determines the current level.
  uint8_t nature = NATURE_HARDY;
  uint8_t ivs[STAT_COUNT] = { 10, 10, 10, 10, 10, 10 };
  uint8_t evs[STAT_COUNT] = { 0, 0, 0, 0, 0, 0 };
  uint8_t bond = 0;
  uint8_t fullness = 80, joy = 80, energy = 80, hygiene = 100;
  uint8_t weight = 0;
  uint8_t careMistakes = 0;
  uint16_t medals = 0;
  bool berryKnown = false;
  uint32_t xp = 0; // Appended last so older party blobs keep their layout.
};

/** Owns pet gameplay state, persistence, progression, and party management. */
class Pet {
public:
  // Care stats range from 0 to 100.
  uint8_t fullness = 80;  // Hunger/fullness.
  uint8_t joy = 80;       // Happiness.
  uint8_t energy = 80;    // Energy.
  uint8_t hygiene = 100;  // Cleanliness.
  uint8_t poops = 0;      // On-screen waste, up to three.
  uint8_t weight = 0;     // Treats increase weight; minigames reduce it.
  // Stat inputs in STAT_* order: IVs are fixed per pet, EVs come from training (max 252 each, 510 total).
  uint8_t nature = NATURE_HARDY;
  uint8_t ivs[STAT_COUNT] = { 10, 10, 10, 10, 10, 10 };
  uint8_t evs[STAT_COUNT] = { 0, 0, 0, 0, 0, 0 };
  bool berryKnown = false;  // Whether the favorite berry has been discovered.
  bool shiny = false;       // Rare color variant rolled when the egg is created.
  uint32_t ageMinutes = 0;
  int16_t speciesId = -1;      // Dex ID (1..DEX_COUNT), or -1 for an egg.
  int16_t prevSpeciesId = -1;  // Previous species during the evolution animation.
  uint8_t careMistakes = 0;   // Each care mistake delays evolution by one level.
  bool sleeping = false;
  uint32_t lastSeenEpoch = 0;   // Last RTC time applied to offline progression.
  uint8_t ceremony = CER_NONE;  // Active farewell, runaway, or release ceremony.
  uint8_t lastEnd = CER_NONE;   // Previous ending, which affects the next egg.
  uint8_t dexReg[DEX_BITMAP_BYTES] = { 0 };       // Raised species.
  uint8_t dexShinyReg[DEX_BITMAP_BYTES] = { 0 };  // Raised or caught shiny species.
  uint8_t dexCaught[DEX_BITMAP_BYTES] = { 0 };    // Caught wild species.
  // Player care streak, retained across pets.
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // Pet bond increases gradually with care and resets for a new pet.
  uint8_t bond = 0;
  uint32_t xp = 0;       // Experience, capped at PET_XP_MAX.
  char nick[12] = "";    // Nickname; empty means use the species name.
  // Medals earned by this pet and the lifetime total across all pets.
  uint16_t medals = 0, totalMedals = 0;
  uint16_t newMedal = 0;   // Newly earned medal bits to celebrate.
  uint16_t lastMilestone = 0;  // Most recently celebrated streak milestone.
  uint16_t gameHi = 0;     // Player's overall minigame record.
  uint16_t strHi = 0;      // Training-bag hit record.
  uint16_t catchHi = 0;    // Catch minigame record.
  uint16_t memoHi = 0;     // Memory minigame record.
  uint16_t cleanHi = 0;    // Cleaning minigame record.
  uint16_t typeHi = 0;     // Type-match minigame record.
  uint16_t battleWins = 0, battleLosses = 0;
  uint16_t battleStreak = 0, bestBattleStreak = 0;
  uint8_t collectionFrame = 0;  // Frame 0 is the default; Dex goals unlock others.
  uint32_t lastPetInteractMinute = 0;
  uint16_t dexRewardMask = 0;
  uint32_t dailyGoalDay = 0;
  uint8_t dailyGoalType[DAILY_GOAL_COUNT] = { DAILY_GOAL_CARE, DAILY_GOAL_PLAY, DAILY_GOAL_CATCH };
  uint8_t dailyGoalProgress[DAILY_GOAL_COUNT] = { 0, 0, 0 };
  uint8_t dailyGoalDone = 0;
  uint8_t itemCounts[EXP_ITEM_COUNT] = { 0 };
  uint32_t expeditionEndEpoch = 0;
  uint8_t expeditionRewardItem = EXP_ITEM_NONE;
  uint8_t expeditionSite = EXP_SITE_NONE;
  uint8_t hoennSealMask = 0;
  uint8_t expeditionMinutes = 0;
  uint16_t expeditionSteps = 0;
  uint8_t expeditionBonusItem = EXP_ITEM_NONE;
  /** Returns the step target for an expedition duration. */
  static uint16_t expeditionStepGoal(uint8_t minutes);
  /** Returns whether the current expedition earned its walking bonus. */
  bool expeditionWalkBonusEarned() const;
  // Player step totals. The daily count resets each RTC day; lifetime steps persist.
  uint32_t stepsToday = 0;
  uint32_t stepsTotal = 0;
  uint32_t stepDay = 0;
  uint8_t stepDailyRewardMask = 0;
  uint8_t stepMilestoneMask = 0;
  bool saveLoadedFromNvs = false;
  bool saveCreatedThisBoot = false;

  // Six-slot party state.
  uint8_t activePartySlot = 0;
  PartySlot party[PARTY_SIZE];

  /** Copies the active pet's state into its current party slot. */
  void syncToParty();
  /** Sets IVs and nature for one party slot, or all occupied slots when slot is -1. Returns the count changed. */
  int setPartyStats(int8_t slot, const uint8_t newIvs[STAT_COUNT], uint8_t newNature);
  /** Sets the active pet's affection (bond, 0-255) and persists it. */
  void setActiveBond(uint8_t value);
  /** Marks the active pet as shiny, registers its shiny variant, and persists the change. */
  bool setActiveShiny();
  bool setActiveEvs(const uint8_t newEvs[STAT_COUNT]);
  bool setActiveSpecies(int16_t dex);
  bool setActiveLevel(uint8_t lv);
  /** Switches the active pet to the requested party slot. */
  bool switchPartySlot(uint8_t slot);
  /** Promotes the next occupied party slot to be the active pet. */
  bool promoteNextFromParty();
  /** Adds a caught species to the party, returning its slot or -1 if full. */
  int8_t addToParty(int16_t dex, bool isShiny, uint8_t lv = 1);
  /** Returns whether all party slots are occupied. */
  bool isPartyFull() const;
  /** Replaces a party slot with the specified species and level. */
  bool replacePartySlot(uint8_t slot, int16_t dex, bool isShiny, uint8_t lv);

  /** Persists the six party slots to their dedicated NVS key. */
  void saveParty();
  /** Loads party slots from NVS. */
  void loadParty();

  /** Loads saved state from NVS or creates the first egg. */
  void begin();
  /** Advances pet simulation and returns whether visible state changed. */
  bool update(uint32_t nowMs);

  // Player actions.
  /** Feeds the pet a red berry for backward compatibility. */
  void feed();
  /** Feeds a berry by color: 0 red, 1 blue, or 2 green. */
  void feedBerry(uint8_t color);
  /** Feeds the pet candy. */
  void feedCandy();
  /** Returns whether the active pet prefers the requested berry color. */
  bool lovesBerry(uint8_t color) const {
    return !isEgg() && (speciesId % 3) == color;
  }
  /** Applies a general minigame result and its speed-training reward. */
  void playResult(uint8_t score);
  /** Applies a catch-minigame result and returns its reward value. */
  uint8_t applyCatchResult(uint8_t score);
  /** Applies a memory-minigame result and returns its reward value. */
  uint8_t applyMemoResult(uint8_t rounds);
  /** Applies a cleaning-minigame result and returns its reward value. */
  uint8_t applyCleanResult(uint8_t score);
  /** Applies a type-match result and returns its reward value. */
  uint8_t applyTypeResult(uint8_t score);
  /** Applies the effect of a random pet interaction event. */
  bool applyPetEvent(uint8_t eventType);
  /** Applies a direct pet interaction and returns the granted effect flags. */
  uint8_t interactPet(bool eveningBonus);
  /** Applies the effect of shaking the device, when allowed. */
  bool applyShake();
  /** Adds walked steps and returns the resulting reward-event flags. */
  uint8_t applyWalk(uint16_t steps, uint32_t nowEpoch = 0);
  /** Returns the step target for a daily milestone index. */
  uint32_t stepGoal(uint8_t index) const;
  /** Returns whether the requested daily step milestone is complete. */
  bool stepGoalComplete(uint8_t index) const;
  /** Returns the player's current trail rank. */
  uint8_t stepTrailRank() const;
  /** Returns the shiny-encounter chance per 4096 step-based rolls. */
  uint16_t stepShinyChancePer4096() const;
  /** Returns the catch-rate bonus currently granted by walking. */
  uint8_t stepCatchBonus() const;
  /** Returns whether a step reward should currently be displayed. */
  bool showStepReward() const { return deadlineActive(millis(), stepRewardUntil); }
  /** Returns the most recently earned step-reward event. */
  uint8_t lastStepReward() const { return lastStepRewardEvent; }
  /** Returns and consumes the pending morning greeting. */
  bool takeMorningGreeting();
  /** Returns the active pet's personality. */
  PetPersonality personality() const;
  /** Ensures that goals for the current day have been initialized. */
  void ensureDailyGoals();
  /** Returns the progress target for a daily goal category. */
  uint8_t dailyGoalTarget(uint8_t goalType) const;
  /** Returns whether the selected daily goal is complete. */
  bool dailyGoalComplete(uint8_t index) const;
  /** Applies training-bag hits and returns the strength gain. */
  uint8_t trainStrength(uint16_t hits);
  /** Applies a battle victory and returns the awarded training reward. */
  BattleReward applyBattleWin(int16_t wildDex, bool closeWin);
  /** Applies battle-loss consequences. */
  void applyBattleLoss();

  // Expedition rewards are rolled at start and persisted to prevent rerolls after
  // a reboot. Random rolls can be injected by native tests.
  /** Returns the energy cost of an expedition of the specified duration. */
  static uint8_t expeditionEnergyCost(uint8_t minutes);
  /** Returns whether an expedition is currently in progress. */
  bool expeditionActive(uint32_t nowEpoch) const;
  /** Returns whether the current expedition is ready to claim. */
  bool expeditionReady(uint32_t nowEpoch) const;
  /** Returns the number of items currently stored in the expedition inventory. */
  uint8_t expeditionItemCount() const;
  /** Returns the state that should be shown by the expedition HUD. */
  ExpeditionHudState expeditionHudState(uint32_t nowEpoch) const;
  /** Returns whether the expedition inventory has reached its capacity. */
  bool expeditionInventoryFull() const;
  /** Checks whether the requested expedition can be started. */
  bool canStartExpedition(uint8_t minutes, uint32_t nowEpoch) const;
  /** Returns the chance of earning a training item for this expedition length. */
  uint8_t expeditionTrainingChance(uint8_t minutes) const;
  /** Starts an expedition using the supplied duration, site, and reward rolls. */
  bool startExpedition(uint8_t minutes, uint32_t nowEpoch, uint8_t luckRoll, uint8_t itemRoll = 0,
                       ExpeditionSite site = EXP_SITE_COAST, uint8_t bonusRoll = 99);
  /** Claims the completed expedition reward, or no item if not ready. */
  ExpeditionItem claimExpedition(uint32_t nowEpoch);
  /** Uses an inventory item and optionally applies it to a training stat. */
  bool useExpeditionItem(ExpeditionItem item, int8_t trainingStat = -1);
  /** Returns the number of Hoenn seals earned. */
  uint8_t hoennSealCount() const;
  /** Returns whether the seal for the specified expedition site has been earned. */
  bool hasHoennSeal(ExpeditionSite site) const;

  // Battle stats combine species base stats, genes, level, and training.
  /** Returns the active pet's calculated attack stat. */
  uint16_t atkStat() const;
  /** Returns the active pet's calculated defense stat. */
  uint16_t defStat() const;
  /** Returns the active pet's calculated speed stat. */
  uint16_t speStat() const;
  /** Returns the active pet's calculated special attack stat. */
  uint16_t spaStat() const;
  /** Returns the active pet's calculated special defense stat. */
  uint16_t spdStat() const;
  /** Returns the final value of a STAT_* index for the active pet. */
  uint16_t statValue(uint8_t statIdx) const;
  /** Same formula as statValue() but with an explicit EV, e.g. 0 to show the base + IV + level value. */
  uint16_t statValueAtEv(uint8_t statIdx, uint8_t ev) const;
  /** Fills out with the inputs and final value of a STAT_* index. Returns false for eggs or invalid indices. */
  bool statBreakdown(uint8_t statIdx, StatBreakdown &out) const;
  /** Returns whether a stat can still receive EVs under the per-stat and total caps. */
  bool evCanGain(uint8_t statIdx) const;
  /** Adds EVs to a stat within the caps and returns the amount actually added. */
  uint8_t addEv(uint8_t statIdx, uint8_t amount);
  /** Starts a play interaction with the pet. */
  void play();
  /** Toggles the pet between sleeping and awake. */
  void toggleLight();
  /** Cleans waste and restores hygiene. */
  void clean();
  /** Applies a petting interaction. */
  void caress();
  /** Advances egg taps and hatches the egg when its tap threshold is reached. */
  void eggTap();
  /** Starts a new randomly selected egg. */
  void newEgg();
  /** Starts the release ceremony for the active pet. */
  void release();
  /** Applies elapsed time since the last saved RTC timestamp. */
  void syncClock(uint32_t nowEpoch);
  /** Sets the pet's clock baseline without applying offline progression. */
  void setClock(uint32_t nowEpoch);
  /** Starts a farewell ceremony for an eligible final-form pet. */
  void startFarewell();
  /** Starts a runaway ceremony for a neglected pet. */
  void startRunaway();

  /** Returns whether the active pet slot contains an egg. */
  bool isEgg() const { return speciesId < 0; }
  /** Returns the number of taps currently applied to the egg. */
  uint8_t eggCracks() const { return eggTaps; }
  /** Returns whether the pet's eating animation is active. */
  bool eating() const { return deadlineActive(millis(), eatUntil); }
  /** Returns whether the pet's heart animation is active. */
  bool showHeart() const { return deadlineActive(millis(), heartUntil); }
  /** Returns whether the evolution animation is active. */
  bool evolving() const { return deadlineActive(millis(), evolveUntil); }
  /** Returns the evolution-animation progress from zero to one. */
  float evolveT() const {
    uint32_t n = millis();
    uint32_t left = deadlineRemaining(n, evolveUntil);
    return 1.0f - (float)left / (float)EVOLVE_ANIM_MS;
  }
  /** Returns whether at least one evolution target is unlocked. */
  bool evolutionUnlocked() const;
  /** Returns whether an evolution level requirement has been reached. */
  bool evolutionLevelReached() const;
  /** Returns whether an otherwise eligible evolution requires more bond. */
  bool evolutionNeedsMoreBond() const;
  /** Returns whether current level, conditions, and care stats permit evolution. */
  bool canEvolveNow() const;
  /** Returns the number of currently available evolution choices. */
  uint8_t evolutionOptionCount() const;
  /** Returns an available evolution target by option index. */
  int16_t evolutionOption(uint8_t index) const;
  /** Returns the required level for the active pet's available evolution. */
  uint8_t evolutionRequiredLevel() const;
  /** Returns the required level for a specific evolution target. */
  uint8_t evolutionRequiredLevelFor(int16_t target) const;
  /** Checks whether the active pet can evolve into the requested species. */
  bool canEvolveTo(int16_t target) const;
  /** Evolves the active pet into the specified target. */
  void evolveTo(int16_t target);
  /** Evolves to the first currently available option. */
  void evolve();
  /** Returns whether the final-form pet is eligible for a farewell. */
  bool canFarewellNow() const;
  /** Returns whether neglect has made the pet eligible to run away. */
  bool canRunawayNow() const;
  // The player can defer evolution or farewell through a choice dialog.
  /** Returns whether the evolution prompt should currently be shown. */
  bool wantEvolveButton() const;
  /** Returns whether the deferred farewell prompt should be shown. */
  bool wantFarewellButton() const { return canFarewellNow() && ageMinutes >= farDeclinedAge; }
  /** Defers the current evolution prompt until a later level. */
  void declineEvolve();
  /** Clears any previously deferred evolution prompt. */
  void resetEvolutionDeferral();
  /** Defers the farewell prompt by one in-game day. */
  void declineFarewell() { farDeclinedAge = ageMinutes + 1440; }
  // On a new game, the player selects the starter species.
  /** Returns whether a new game is waiting for starter selection. */
  bool awaitingStarter() const { return starterPick; }
  /** Selects the starter species and persists the new game state. */
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = false; save(); }
  /** Clears all saved pet data from NVS; intended for local test commands. */
  void factoryReset() { prefs.clear(); }
  /** Sets care stats and neglect time so tests can trigger the runaway state. */
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; }
  /** Returns the pet's level, capped at 100. */
  uint8_t level() const {
    uint32_t raw = 1UL + ageMinutes / MINUTES_PER_LEVEL;
    return raw > 100UL ? 100 : (uint8_t)raw;
  }
  /** Returns whether the species is registered as raised. */
  bool isRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  /** Returns whether the species is registered as caught. */
  bool isCaught(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexCaught[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  /** Returns whether a shiny version of the species is registered. */
  bool isShinyRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT && (dexShinyReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  /** Returns the number of registered raised species. */
  uint16_t registeredCount() const;
  /** Returns the number of registered caught species. */
  uint16_t caughtCount() const;
  /** Returns the number of species known through raising or catching. */
  uint16_t knownDexCount() const;
  /** Returns the current collection rank. */
  uint8_t collectionRank() const;
  /** Returns the number of collection frames currently unlocked. */
  uint8_t unlockedCollectionFrameCount() const;
  /** Selects an unlocked collection frame. */
  bool setCollectionFrame(uint8_t frame);
  /** Registers a caught species and updates related collection progress. */
  void registerCaught(int16_t dex, bool shinyVariant = false, uint8_t wildLevel = 1);
  /** Returns the next Dex collection milestone. */
  uint16_t nextDexGoal() const;
  /** Applies any unclaimed Dex collection rewards and returns the awarded mask. */
  uint16_t applyDexRewards();
  /** Calculates the chance to catch a wild species after a battle. */
  uint8_t catchChanceForWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin) const;
  /** Calculates the chance to catch a wild species using the respect mechanic. */
  uint8_t respectCatchChanceForWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel) const;
  /** Attempts a normal wild catch using the supplied deterministic roll. */
  bool tryCatchWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel, bool closeWin,
                    uint8_t luckRoll, bool shinyVariant = false);
  /** Attempts a respect-based wild catch using the supplied deterministic roll. */
  bool tryRespectCatchWild(int16_t wildDex, uint8_t wildLevel, uint8_t petLevel,
                           uint8_t luckRoll, bool shinyVariant = false);
  /** Returns whether an evolution line contains an unregistered species. */
  bool lineHasUnregistered(int16_t base) const;
  /** Returns whether the species has any defined evolution path. */
  bool hasEvolutionPath(int16_t dex) const;
  /** Returns the current egg's rarity without revealing its species. */
  uint8_t eggRarity() const;
  /** Selects and returns a species for a new egg. */
  int16_t pickEggSpecies();
  /** Counts care stats currently above the healthy threshold. */
  uint8_t healthyStatCount() const {
    return (uint8_t)((fullness > 40 ? 1 : 0) + (joy > 40 ? 1 : 0) +
                     (energy > 40 ? 1 : 0) + (hygiene > 40 ? 1 : 0));
  }
  /** Returns the lowest of the pet's care stats. */
  uint8_t lowestStat() const { return min(min(fullness, joy), min(energy, hygiene)); }
  /** Returns the mood derived from the pet's current state. */
  PetMood mood() const;
  /** Returns farewell/runaway ceremony progress from zero to one. */
  float ceremonyT() const {
    if (ceremony == CER_NONE) return 0.0f;
    uint32_t n = millis();
    uint32_t left = deadlineRemaining(n, ceremonyUntil);
    return 1.0f - (float)left / (float)CEREMONY_MS;
  }

  // Streak, bond, medals, and nickname.
  /** Replaces the pet nickname and persists the change. */
  void rename(const char *name);
  /** Returns whether the pet has earned the specified medal bit. */
  bool hasMedal(uint16_t m) const { return medals & m; }
  /** Returns whether the medal celebration is active. */
  bool showMedal() const { return deadlineActive(millis(), medalUntil); }
  /** Returns whether the streak-milestone celebration is active. */
  bool showMilestone() const { return deadlineActive(millis(), milestoneUntil); }
  /** Returns whether a Dex reward celebration is active. */
  bool showDexReward() const { return deadlineActive(millis(), dexRewardUntil); }
  /** Returns the Dex milestone associated with the latest reward. */
  uint16_t lastDexRewardGoal() const { return lastDexReward; }
  /** Returns the egg-quality bonus from the care streak and bond. */
  int careBonus() const;

  // Deferred periodic saves are flushed when the display is dimmed or off.
  /** Returns whether a periodic save is waiting to be written. */
  bool savePending() const { return pendingSave; }
  /** Writes pending pet state to NVS. */
  void flushSave();
  // Versioned game-state backup for the browser USB tool; SD data is excluded.
  static constexpr size_t BACKUP_MAX_BYTES = 512;
  /** Serializes pet state into a bounded versioned backup buffer. */
  size_t exportBackup(uint8_t *out, size_t capacity) const;
  /** Validates and imports a versioned pet-state backup. */
  bool importBackup(const uint8_t *data, size_t length);

private:
  Preferences prefs;
  uint32_t lastTick = 0;
  uint32_t eatUntil = 0;
  uint32_t heartUntil = 0;
  uint32_t evolveUntil = 0;
  int16_t eggTarget = 1;       // Hidden species selected for the egg.
  bool eggShiny = false;       // Shiny variant rolled when the egg is created.
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // Periodic save waiting to be written.
  uint8_t evoDeclinedLv = 0;    // Level at which the evolution prompt was deferred.
  uint32_t evoDeclinedAge = 0;  // Age threshold for deferral at level 100.
  uint32_t farDeclinedAge = 0;  // Age threshold for showing a deferred farewell.
  bool starterPick = false;     // New game is waiting for starter selection.
  uint8_t neglectTicks = 0;
  uint16_t goodTicks = 0;  // Consecutive care ticks contribute to defense.
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // Daily cap tracking for bond gains.
  uint32_t medalUntil = 0;     // End time of the medal celebration.
  uint32_t milestoneUntil = 0; // End time of the streak celebration.
  uint32_t dexRewardUntil = 0;
  uint16_t lastDexReward = 0;
  uint32_t lastMorningDay = 0;
  uint32_t shakeReadyAt = 0;
  uint32_t shakeDay = 0;
  uint8_t shakeCountToday = 0;
  uint16_t walkJoyBank = 0;
  uint16_t walkBondBank = 0;
  uint32_t walkDay = 0;
  uint32_t walkHour = 0;
  uint8_t walkJoyToday = 0;
  uint8_t walkJoyHour = 0;
  uint8_t walkBondToday = 0;
  uint32_t stepRewardUntil = 0;
  uint8_t lastStepRewardEvent = STEP_REWARD_NONE;
  uint8_t pendingStepRewardMask = 0;

  /** Returns the UTC day containing the last applied RTC timestamp. */
  uint32_t today() const { return lastSeenEpoch ? lastSeenEpoch / 86400 : 0; }
  /** Records the first care action of the day for streak and bond progress. */
  void registerCare();
  /** Adds bond, respecting the per-day gain limit. */
  void addBond(uint8_t amt);
  /** Adds progress to matching daily goals. */
  void noteDailyGoal(uint8_t goalType, uint8_t amount);
  /** Applies rewards for completed daily goals. */
  void applyDailyReward();
  /** Records a completed daily step milestone. */
  void recordStepReward(uint8_t index);
  /** Applies step rewards accumulated while the daily step state was pending. */
  void applyPendingStepRewards();
  /** Resets daily step counters and reward masks when the RTC day changes. */
  void ensureStepDay();
  /** Awards any medals whose requirements have been met. */
  void checkMedals();
  /** Advances one simulation tick of pet care and progression. */
  void tick();
  /** Hatches the current egg and initializes its pet state. */
  void hatch();
  /** Registers a raised species and updates Dex rewards. */
  void registerSpecies(int16_t dex);
  /** Checks whether the inventory can accept the specified expedition item. */
  bool canReceiveExpeditionItem(ExpeditionItem item) const;
  /** Selects an expedition reward from its site, duration, and random rolls. */
  ExpeditionItem expeditionRewardFor(ExpeditionSite site, uint8_t minutes, uint8_t luckRoll, uint8_t itemRoll) const;
  /** Persists the current pet state to NVS. */
  void save();
  /** Loads pet state from NVS. */
  void load();
  /** Clamps an integer value to the inclusive range from zero to 100. */
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
