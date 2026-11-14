#pragma once
#include "screen.h"
#include <stdint.h>
#include "../../i18n.h"

class MinigameScreen : public Screen {
protected:
  uint8_t gameScore = 0;
  uint8_t gameMisses = 0;
  uint32_t gameOverUntil = 0;
  bool gameNewHi = false;
  uint8_t gameGain = 0;
  float hitX = 0;
  float hitY = 0;
  uint32_t hitTime = 0;
  bool closeRequested = false;

  /** Clears per-session score, timing, and feedback state. */
  void resetGame();
  /** Displays the result overlay for the requested duration. */
  void showResult(uint32_t durationMs = 4000);
  /** Requests that the active minigame screen be closed. */
  void closeGame();
  /** Draws the shared score, record, and reward result panel. */
  void renderResult(const char *recordFmt, uint16_t record, StrId gainFmt);

public:
  /** Identifies this screen as a minigame. */
  bool isMinigame() const override { return true; }
  /** Returns whether the minigame has requested to close. */
  bool shouldClose() const override { return closeRequested; }
};
