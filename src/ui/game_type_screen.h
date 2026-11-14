#pragma once
#include "../../battle.h"
#include "../../dex.h"
#include "game_screen_common.h"

class GameTypeScreen : public MinigameScreen {
  uint8_t typeEnemy = TYPE_GRASS;
  uint8_t typeChoice[3] = { TYPE_FIRE, TYPE_WATER, TYPE_GRASS };
  uint8_t typeCorrect = 0;
  uint32_t typeUntil = 0;

  /** Chooses and displays the next type-match question. */
  void nextQuestion();
  /** Stores the score and completes the type-match minigame. */
  void finishGame();

public:
  /** Initializes a new type-match minigame session. */
  void start();
  /** Question timers require rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Draws the current type matchup and answer choices. */
  void render() override;
  /** Checks the selected type answer. */
  void handleTap(int16_t x, int16_t y) override;
};

extern GameTypeScreen gameTypeScreen;
