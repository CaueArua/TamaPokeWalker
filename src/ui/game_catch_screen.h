#pragma once
#include "game_screen_common.h"

class GameCatchScreen : public MinigameScreen {
  uint32_t catchUntil = 0, catchTargetUntil = 0;
  int16_t catchX = 0, catchY = 0;
  uint8_t catchIcon = 0;

  /** Places a new catch target and starts its visibility timer. */
  void spawnTarget();
  /** Stores the score and completes the catch minigame. */
  void finishGame();

public:
  /** Initializes a new catch minigame session. */
  void start();
  /** Target motion and timers require rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Handles the player's attempt to catch the visible target. */
  bool handlesTapOnPress() const override { return true; }
  /** Draws the catch minigame and current target. */
  void render() override;
  /** Resolves a tap against the current target. */
  void handleTap(int16_t x, int16_t y) override;
};

extern GameCatchScreen gameCatchScreen;
