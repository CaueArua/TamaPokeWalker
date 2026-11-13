#include "game_screen_common.h"
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>
#include "../../pet.h"
#include "../../species.h"
#include "../../time_utils.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool gameOpen;
extern bool gameNewHi;
extern uint8_t gameScore;
extern uint8_t gameGain;
extern uint32_t gameOverUntil;
extern void drawGameScene();
extern int sceneHour();
extern void flushStampedFrame();

static constexpr int GAME_RESULT_CX = 233;

void drawGameResult(const char *recordFmt, uint16_t record, StrId gainFmt) {
  drawGameScene();
  if (deadlineReached(millis(), gameOverUntil)) {
    gameOpen = false;
    return;
  }
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  char buf[22];
  snprintf(buf, sizeof(buf), T(S_SCORE_FMT), gameScore);
  gfx->setTextColor(ink);
  gfx->setTextSize(4);
  gfx->setCursor(GAME_RESULT_CX - strlen(buf) * 12, 148);
  gfx->print(buf);
  char gain[18];
  snprintf(gain, sizeof(gain), T(gainFmt), gameGain);
  gfx->setTextColor(gainFmt == S_DEF_GAIN_FMT ? 0x4C98 : (gainFmt == S_HYG_GAIN_FMT ? UI_BAR_OK : UI_BAR_WARN));
  gfx->setTextSize(3);
  gfx->setCursor(GAME_RESULT_CX - strlen(gain) * 9, 204);
  gfx->print(gain);
  gfx->setTextSize(2);
  if (gameNewHi && gameScore > 0) {
    gfx->setTextColor(UI_BAR_WARN);
    gfx->setCursor(GAME_RESULT_CX - strlen(T(S_NEW_RECORD)) * 6, 256);
    gfx->print(T(S_NEW_RECORD));
  } else {
    char rec[20];
    snprintf(rec, sizeof(rec), recordFmt, record);
    gfx->setTextColor(ink);
    gfx->setCursor(GAME_RESULT_CX - strlen(rec) * 6, 256);
    gfx->print(rec);
  }
  flushStampedFrame();
}
