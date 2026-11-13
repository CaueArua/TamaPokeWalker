#include "game_catch_screen.h"
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
extern uint8_t gameMisses;
extern uint32_t gameOverUntil;
extern int16_t catchX, catchY;
extern uint8_t catchIcon;
extern uint32_t catchUntil, catchTargetUntil;
extern float hitX, hitY;
extern uint32_t hitTime;

extern void drawGameScene();
extern void finishCatchGame();
extern void spawnCatchTarget();
extern void flushStampedFrame();
extern int sceneHour();
extern void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette);

GameCatchScreen gameCatchScreen;

static constexpr int CATCH_CX = 233;

void GameCatchScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) { gameOpen = false; sfxPlay(SFX_TAP); return; }
  int dx = x - catchX, dy = y - catchY;
  if (dx * dx + dy * dy <= 52 * 52) {
    gameScore++;
    hitX = catchX;
    hitY = catchY;
    hitTime = millis();
    sfxPlay(SFX_MINIGAME_OK);
    spawnCatchTarget();
  } else if (++gameMisses >= 3) {
    finishCatchGame();
  } else {
    sfxPlay(SFX_MINIGAME_BAD);
  }
}

void GameCatchScreen::render() {
  uint32_t now = millis();
  if (gameOverUntil) {
    drawGameResult(T(S_RECORD_FMT), pet.catchHi, S_SPD_GAIN_FMT);
    return;
  }
  if (deadlineReached(now, catchUntil) || gameMisses >= 3) {
    finishCatchGame();
    return;
  }
  if (deadlineReached(now, catchTargetUntil)) {
    if (++gameMisses >= 3) {
      finishCatchGame();
      return;
    }
    spawnCatchTarget();
  }
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  gfx->setTextColor(ink);
  gfx->setTextSize(3);
  gfx->setCursor(CATCH_CX - strlen(T(S_CATCH_TITLE)) * 9, 32);
  gfx->print(T(S_CATCH_TITLE));
  char score[16], rec[16];
  snprintf(score, sizeof(score), T(S_SCORE_FMT), gameScore);
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.catchHi);
  gfx->setTextSize(2);
  gfx->setCursor(50, 78);
  gfx->print(score);
  gfx->setCursor(294, 78);
  gfx->print(rec);
  for (int i = 0; i < 3; i++) {
    if (i < 3 - gameMisses) gfx->fillCircle(180 + i * 28, 104, 6, UI_BAR_BAD);
    else gfx->drawCircle(180 + i * 28, 104, 6, UI_TRACK);
  }
  const char *const *icon = catchIcon == 0 ? SPR_ICON_FOOD : (catchIcon == 1 ? SPR_ICON_BERRY_B : SPR_ICON_BERRY_G);
  gfx->fillCircle(catchX, catchY, 34, UI_WHITE);
  gfx->drawCircle(catchX, catchY, 36, UI_BAR_WARN);
  drawMap(icon, 16, catchX - 24, catchY - 24, 3, false);
  int bw = 280;
  int fw = (int)((uint32_t)bw * (catchUntil - now) / 20000);
  if (fw < 0) fw = 0;
  gfx->fillRoundRect(CATCH_CX - bw / 2, 362, bw, 16, 5, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(CATCH_CX - bw / 2, 362, fw, 16, 5, UI_BAR_OK);
  uint32_t ht = millis() - hitTime;
  if (hitTime && ht < 220) gfx->drawCircle((int)hitX, (int)hitY, 42 + ht / 8, UI_BAR_WARN);
  flushStampedFrame();
}
