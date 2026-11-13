#include "game_type_screen.h"
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../battle.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../species.h"
#include "../../time_utils.h"
#include "game_screen_common.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool gameOpen;
extern uint8_t gameScore, gameMisses;
extern uint8_t typeEnemy, typeChoice[3], typeCorrect;
extern uint32_t typeUntil, gameOverUntil;

extern void drawGameScene();
extern void finishTypeGame();
extern void nextTypeQuestion();
extern void flushStampedFrame();
extern int sceneHour();
extern const char *battleTypeName(uint8_t type);
extern uint16_t battleTypeColor(uint8_t type);
extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);

GameTypeScreen gameTypeScreen;

static constexpr int TYPE_CX = 233;

void GameTypeScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) { gameOpen = false; sfxPlay(SFX_TAP); return; }
  int idx = -1;
  for (int i = 0; i < 3; i++) {
    int by = 210 + i * 60;
    if (x >= 70 && x <= 396 && y >= by - 8 && y <= by + 56) idx = i;
  }
  if (idx < 0) return;
  if ((uint8_t)idx == typeCorrect) {
    gameScore++;
    sfxPlay(SFX_MINIGAME_OK);
    nextTypeQuestion();
  } else {
    if (++gameMisses >= 3) finishTypeGame();
    else sfxPlay(SFX_MINIGAME_BAD);
  }
}

void GameTypeScreen::render() {
  uint32_t now = millis();
  if (gameOverUntil) {
    drawGameResult(T(S_RECORD_FMT), pet.typeHi, S_ATK_GAIN_FMT);
    return;
  }
  if (deadlineReached(now, typeUntil)) {
    if (++gameMisses >= 3) {
      finishTypeGame();
      return;
    }
    sfxPlay(SFX_MINIGAME_BAD);
    nextTypeQuestion();
  }
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  gfx->setTextColor(ink);
  gfx->setTextSize(3);
  gfx->setCursor(TYPE_CX - strlen(T(S_TYPE_TITLE)) * 9, 32);
  gfx->print(T(S_TYPE_TITLE));
  char score[16], rec[16];
  snprintf(score, sizeof(score), T(S_SCORE_FMT), gameScore);
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.typeHi);
  gfx->setTextSize(2);
  gfx->setCursor(50, 78);
  gfx->print(score);
  gfx->setCursor(294, 78);
  gfx->print(rec);
  for (int i = 0; i < 3; i++) {
    if (i < 3 - gameMisses) gfx->fillCircle(180 + i * 28, 104, 6, UI_BAR_BAD);
    else gfx->drawCircle(180 + i * 28, 104, 6, UI_TRACK);
  }

  const char *enemy = battleTypeName(typeEnemy);
  gfx->fillRoundRect(118, 126, 230, 54, 14, lerp565(battleTypeColor(typeEnemy), UI_WHITE, 4, 8));
  gfx->drawRoundRect(118, 126, 230, 54, 14, ink);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(TYPE_CX - strlen(enemy) * 9, 143);
  gfx->print(enemy);

  for (int i = 0; i < 3; i++) {
    int bx = 88;
    int by = 210 + i * 60;
    const char *label = battleTypeName(typeChoice[i]);
    gfx->fillRoundRect(bx, by, 290, 48, 12, lerp565(battleTypeColor(typeChoice[i]), UI_WHITE, 5, 8));
    gfx->drawRoundRect(bx, by, 290, 48, 12, ink);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(bx + (290 - (int)strlen(label) * 12) / 2, by + 17);
    gfx->print(label);
  }
  int bw = 280;
  int fw = (int)((uint32_t)bw * (typeUntil - now) / 4200);
  if (fw < 0) fw = 0;
  gfx->fillRoundRect(TYPE_CX - bw / 2, 392, bw, 14, 5, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(TYPE_CX - bw / 2, 392, fw, 14, 5, UI_BAR_OK);
  flushStampedFrame();
}
