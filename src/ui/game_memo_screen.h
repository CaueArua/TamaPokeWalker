#pragma once
#include "game_screen_common.h"

class GameMemoScreen : public MinigameScreen {
  uint8_t memoSeq[14] = {};
  uint8_t memoLen = 0, memoShow = 0, memoInput = 0, memoRounds = 0;
  uint32_t memoNextAt = 0;
  bool memoShowing = false;
  int8_t memoActivePad = -1, memoFlashPad = -1, memoHintPad = -1;
  bool memoFlashGood = false;
  uint32_t memoFlashUntil = 0, memoFailUntil = 0, memoTurnUntil = 0;

  /** Appends and displays the next sequence for the current round. */
  void startRound();
  /** Advances sequence playback, input timing, and round state. */
  void stepGame();
  /** Plays the sound associated with a sequence pad. */
  void padSound(uint8_t pad);
  /** Stores the completed-round score and ends the minigame. */
  void finishGame();

public:
  /** Initializes a new memory-sequence minigame session. */
  void start();
  /** Sequence playback and timers require rendering on every update. */
  bool isStaticClean() const override { return false; }
  /** Draws the sequence pads and current playback/input feedback. */
  void render() override;
  /** Checks the player's pad input against the displayed sequence. */
  void handleTap(int16_t x, int16_t y) override;
};

extern GameMemoScreen gameMemoScreen;
