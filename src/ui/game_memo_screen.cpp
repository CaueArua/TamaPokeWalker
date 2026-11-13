#include "game_memo_screen.h"
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../species.h"
#include "../../time_utils.h"
#include "game_screen_common.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool gameOpen;
extern uint8_t gameScore;
extern uint8_t memoSeq[14];
extern uint8_t memoLen, memoShow, memoInput, memoRounds;
extern uint32_t memoNextAt;
extern bool memoShowing;
extern int8_t memoActivePad, memoFlashPad, memoHintPad;
extern bool memoFlashGood;
extern uint32_t memoFlashUntil, memoFailUntil, memoTurnUntil;
extern uint32_t gameOverUntil;

extern void drawGameScene();
extern void stepMemoGame();
extern void memoPadSound(uint8_t pad);
extern void startMemoRound();
extern void finishMemoGame();
extern void flushStampedFrame();
extern int sceneHour();
extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);

GameMemoScreen gameMemoScreen;

static constexpr int MEMO_CX = 233;

static int memoPadAt(int16_t x, int16_t y) {
  const int16_t px[4] = { 142, 324, 142, 324 };
  const int16_t py[4] = { 164, 164, 318, 318 };
  for (int i = 0; i < 4; i++) {
    int dx = x - px[i], dy = y - py[i];
    if (dx * dx + dy * dy <= 54 * 54) return i;
  }
  return -1;
}

void GameMemoScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) { gameOpen = false; sfxPlay(SFX_TAP); return; }
  if (memoShowing || memoFailUntil || deadlineActive(millis(), memoTurnUntil)) return;
  int pad = memoPadAt(x, y);
  if (pad < 0) return;
  if (pad != memoSeq[memoInput]) {
    memoFlashPad = pad;
    memoFlashGood = false;
    memoFlashUntil = millis() + 620;
    memoHintPad = memoSeq[memoInput];
    memoFailUntil = memoFlashUntil;
    sfxPlay(SFX_MINIGAME_BAD);
    return;
  }
  memoFlashPad = pad;
  memoFlashGood = true;
  memoFlashUntil = millis() + 180;
  memoPadSound((uint8_t)pad);
  memoInput++;
  if (memoInput >= memoLen) {
    memoRounds++;
    if (memoLen >= 14) finishMemoGame();
    else startMemoRound();
  }
}

void GameMemoScreen::render() {
  if (gameOverUntil) {
    drawGameResult(T(S_RECORD_FMT), pet.memoHi, S_DEF_GAIN_FMT);
    return;
  }
  stepMemoGame();
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  char roundBuf[18], rec[16];
  snprintf(roundBuf, sizeof(roundBuf), T(S_ROUND_FMT), memoRounds + 1);
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.memoHi);
  gfx->setTextColor(ink);
  gfx->setTextSize(3);
  gfx->setCursor(MEMO_CX - strlen(T(S_GAME_MEMO)) * 9, 34);
  gfx->print(T(S_GAME_MEMO));
  gfx->setTextSize(2);
  gfx->setCursor(60, 82);
  gfx->print(roundBuf);
  gfx->setCursor(310, 82);
  gfx->print(rec);
  const int16_t px[4] = { 142, 324, 142, 324 };
  const int16_t py[4] = { 164, 164, 318, 318 };
  const uint16_t col[4] = { UI_BAR_BAD, UI_BAR_WARN, 0x4C98, UI_BAR_OK };
  int active = memoShowing ? memoActivePad : (memoFailUntil ? memoHintPad : -1);
  for (int i = 0; i < 4; i++) {
    uint16_t fill = i == active ? lerp565(col[i], UI_WHITE, 5, 8) : col[i];
    gfx->fillCircle(px[i], py[i], 48, fill);
    gfx->drawCircle(px[i], py[i], 52, ink);
    if (i == active) {
      int pulse = 56 + (int)((millis() / 70) % 5);
      gfx->drawCircle(px[i], py[i], pulse, col[i]);
    }
    if (i == memoFlashPad && deadlineActive(millis(), memoFlashUntil)) {
      gfx->drawCircle(px[i], py[i], 60, memoFlashGood ? UI_BAR_OK : UI_BAR_BAD);
      gfx->drawCircle(px[i], py[i], 64, memoFlashGood ? UI_BAR_OK : UI_BAR_BAD);
    }
  }
  char phase[28];
  if (memoFailUntil) snprintf(phase, sizeof(phase), "%s", T(S_MEMO_WRONG));
  else if (memoShowing) snprintf(phase, sizeof(phase), "%s", T(S_MEMO_WATCH));
  else snprintf(phase, sizeof(phase), T(S_MEMO_TURN_FMT), memoInput + 1, memoLen);
  gfx->fillRoundRect(78, 230, 310, 24, 7, night ? UI_BG_NIGHT : UI_BG_DAY);
  gfx->drawRoundRect(78, 230, 310, 24, 7, ink);
  gfx->setTextColor(memoFailUntil ? UI_BAR_BAD : (memoShowing ? UI_BAR_WARN : UI_BAR_OK));
  gfx->setTextSize(2);
  gfx->setCursor(MEMO_CX - (int)strlen(phase) * 6, 234);
  gfx->print(phase);
  flushStampedFrame();
}
