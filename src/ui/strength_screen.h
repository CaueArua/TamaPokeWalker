#pragma once
#include "screen.h"
#include "../../pet.h"

class StrengthTrainingScreen : public Screen {
private:
  uint32_t sackUntil = 0;
  uint32_t sackOverUntil = 0;
  uint16_t sackHits = 0;
  float sackShake = 0;
  uint8_t sackGain = 0;
  bool sackNewHi = false;
  Screen *returnScreen = nullptr;
  /** Closes the training screen and returns to its caller. */
  void close();

public:
  /** Opens and initializes the strength-training minigame. */
  void open(Screen *returnTo = nullptr);
  /** Counts a press as a training hit. */
  void handlePress(int16_t y) override;

  /** Resets the training bag when the screen becomes active. */
  void onEnter() override;
  /** The training animation requires rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Draws the training bag and current score. */
  void render() override;
};

extern StrengthTrainingScreen strengthTrainingScreen;
