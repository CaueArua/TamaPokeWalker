#pragma once
#include "game_screen_common.h"

class GameCleanScreen : public MinigameScreen {
  uint8_t cleanActive = 0;
  int16_t cleanX[4] = {};
  int16_t cleanY[4] = {};
  bool cleanAlive[4] = {};
  uint32_t cleanUntil = 0, cleanSpawnAt = 0;

  /** Adds a dirt spot to an available position. */
  void spawnSpot();
  /** Stores the score and completes the cleaning minigame. */
  void finishGame();

public:
  /** Initializes a new cleaning minigame session. */
  void start();
  /** Spot spawning and lifetime timers require rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Draws the cleaning minigame and active dirt spots. */
  void render() override;
  /** Cleans a spot hit by the player's tap. */
  void handleTap(int16_t x, int16_t y) override;
};

extern GameCleanScreen gameCleanScreen;
