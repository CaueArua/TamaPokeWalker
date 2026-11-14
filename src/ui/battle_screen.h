#pragma once
#include <stddef.h>
#include "../../battle.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "screen.h"

class BattleScreen : public Screen {
  BattleStats battlePlayer = {};
  BattleStats battleEnemy = {};
  BattleResult battleResult = {};
  BattleRuntime battleRun = {};
  BattleTurnResult battleTurn = {};
  BattleAction battleLastAction = BATTLE_ATTACK;
  BattleReward battleReward = {};
  char battleMsg[28] = "";
  int16_t battleDex = 0;
  uint8_t battleLevel = 1;
  bool battleShiny = false;
  bool battleResolved = false;
  bool battleDirty = true;
  uint32_t battleAttackMenuUntil = 0;
  bool battleCatchOffered = false;
  bool battleCatchTried = false;
  bool battleCatchDone = false;
  bool battleCatchSuccess = false;
  bool battleRespectCatch = false;
  uint8_t battleCatchChance = 0;
  bool battleLowHpWarned = false;
  Screen *returnScreen = nullptr;
  PmdMon wildPmd;

  /** Builds battle stats from the active pet's current attributes. */
  BattleStats petBattleStats() const;
  /** Closes the battle and returns to the screen that opened it. */
  void closeBattle();
  /** Applies the completed battle result and updates pet records/rewards. */
  void finishBattle();
  /** Executes the selected player action and updates the battle state. */
  void performBattleAction(BattleAction action);
  /** Formats the reward message into the supplied buffer. */
  void battleRewardText(char *buf, size_t len) const;

public:
  /** Starts a wild battle, optionally forcing opponent species, level, and form. */
  void start(int16_t forcedDex = 0, uint8_t forcedLevel = 0, int8_t forcedShiny = -1);
  /** Returns whether the battle has been resolved. */
  bool isResolved() const { return battleResolved; }
  /** Returns whether the battle screen needs to be redrawn. */
  bool isDirty() const { return battleDirty; }
  /** Returns the opponent's Dex number. */
  int16_t opponentDex() const { return battleDex; }
  /** Returns whether the opponent is shiny. */
  bool opponentShiny() const { return battleShiny; }
  /** Marks the battle screen as needing a redraw. */
  void markDirty() override { battleDirty = true; }
  /** Returns whether the battle screen can skip rendering. */
  bool isStaticClean() const override;
  /** Draws the current battle state. */
  void render() override;
  /** Routes a tap to the battle menu or active battle controls. */
  void handleTap(int16_t x, int16_t y) override;
};

extern BattleScreen battleScreen;
