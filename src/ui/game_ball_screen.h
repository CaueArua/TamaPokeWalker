#pragma once
#include "game_screen_common.h"

class GameBallScreen : public MinigameScreen {
  float ballX = 0, ballY = 0, ballVX = 0, ballVY = 0, gamePetX = 0;
  uint32_t ballLastHitAt = 0;

  /** Resets the ball position and velocity for the next play. */
  void respawnBall();
  /** Returns the ball's hit radius at its current position. */
  int ballHitRadius() const;
  /** Advances ball physics and resolves collisions and game timing. */
  void stepGame();

public:
  /** Initializes a new ball minigame session. */
  void start();
  /** Ball physics require rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Starts the swing when the screen receives a press. */
  bool handlesTapOnPress() const override { return true; }
  /** Draws the ball minigame. */
  void render() override;
  /** Handles the player's ball hit input. */
  void handleTap(int16_t x, int16_t y) override;
};

extern GameBallScreen gameBallScreen;
