#include "game_ball_screen.h"
#include <Arduino_GFX_Library.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../species.h"
#include "../../time_utils.h"
#include "game_screen_common.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon pmd;
extern SdMon mon;
extern bool gameOpen;
extern bool gameNewHi;
extern uint8_t gameScore;
extern uint8_t gameMisses;
extern uint32_t gameOverUntil;
extern float ballX, ballY, ballVX, ballVY, gamePetX;
extern float hitX, hitY;
extern uint32_t hitTime, ballLastHitAt;

extern void drawGameScene();
extern void stepGame();
extern int ballHitRadius();
extern int sceneHour();
extern void flushStampedFrame();
extern void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette);
extern void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t,
                       bool loop, bool sil, uint8_t maxS);

GameBallScreen gameBallScreen;

static constexpr int BALL_CX = 233;

static uint16_t ballColor565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
}

void GameBallScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) {
    gameOpen = false;
    sfxPlay(SFX_TAP);
    return;
  }
  float dx = ballX - x, dy = ballY - y;
  int hitRadius = ballHitRadius();
  if (dx * dx + dy * dy < hitRadius * hitRadius) {
    uint32_t now = millis();
    if (now - ballLastHitAt < 260 || ballVY < -0.25f) return;
    gameScore++;
    sfxPlay(SFX_MINIGAME_OK);
    float lift = 4.65f + (gameScore > 16 ? 1.7f : gameScore * 0.11f);
    ballVY = -lift;
    float drift = 0.38f + (gameScore >= 6 ? 0.08f : 0.0f) + (gameScore >= 14 ? 0.10f : 0.0f);
    float chaos = ((int)random(17) - 8) * 0.26f;
    ballVX += dx * drift + chaos;
    if (random(100) < 28) ballVX = -ballVX * (0.72f + random(45) * 0.01f);
    if (ballVX > 11.5f) ballVX = 11.5f;
    if (ballVX < -11.5f) ballVX = -11.5f;
    hitX = ballX;
    hitY = ballY;
    hitTime = now;
    ballLastHitAt = now;
  }
}

void GameBallScreen::render() {
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;

  if (gameOverUntil) {
    drawGameScene();
    if (deadlineReached(millis(), gameOverUntil)) {
      gameOpen = false;
      return;
    }
    char buf[22];
    snprintf(buf, sizeof(buf), T(S_SCORE_FMT), gameScore);
    gfx->setTextColor(ink);
    gfx->setTextSize(4);
    gfx->setCursor(BALL_CX - strlen(buf) * 12, 160);
    gfx->print(buf);
    gfx->setTextSize(2);
    if (gameNewHi && gameScore > 0) {
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setCursor(BALL_CX - strlen(T(S_NEW_RECORD)) * 6, 214);
      gfx->print(T(S_NEW_RECORD));
    } else {
      char rec[20];
      snprintf(rec, sizeof(rec), T(S_RECORD_FMT), pet.gameHi);
      gfx->setTextColor(ink);
      gfx->setCursor(BALL_CX - strlen(rec) * 6, 214);
      gfx->print(rec);
    }
    const char *msg = gameScore >= 10 ? T(S_GREAT_JOY) : T(S_PLUS_JOY);
    gfx->setTextColor(ink);
    gfx->setCursor(BALL_CX - strlen(msg) * 6, 250);
    gfx->print(msg);
    flushStampedFrame();
    return;
  }

  drawGameScene();
  stepGame();

  char buf[8];
  snprintf(buf, sizeof(buf), "%u", gameScore);
  gfx->setTextColor(ink);
  gfx->setTextSize(4);
  gfx->setCursor(BALL_CX - strlen(buf) * 12, 30);
  gfx->print(buf);
  char rec[12];
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.gameHi);
  gfx->setTextSize(2);
  gfx->setCursor(BALL_CX - strlen(rec) * 6, 76);
  gfx->print(rec);
  for (int i = 0; i < 3; i++) {
    if (i < 3 - gameMisses) gfx->fillCircle(180 + i * 28, 104, 6, UI_BAR_BAD);
    else gfx->drawCircle(180 + i * 28, 104, 6, UI_TRACK);
  }

  if (pmd.loaded) {
    uint8_t act = (ballX > gamePetX + 4) ? PMD_WALKR : (ballX < gamePetX - 4) ? PMD_WALKL : PMD_IDLE;
    if (!pmd.has(act)) act = PMD_IDLE;
    drawPmdAct(act, (int)gamePetX, 394, millis(), true, false, 3);
  } else if (mon.loaded) {
    int s = (mon.h * 2 > 130) ? 1 : 2;
    int w = mon.w * s, h = mon.h * s;
    uint16_t fm = mon.frameMs ? mon.frameMs : 100;
    uint16_t fi = (millis() / fm) % mon.frames;
    const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
    int px = (int)gamePetX - w / 2, py = 394 - h;
    for (int r = 0; r < mon.h; r++)
      for (int c = 0; c < mon.w; c++) {
        uint8_t idx = fr[r * mon.w + c];
        if (idx == 0xFF) continue;
        gfx->fillRect(px + c * s, py + r * s, s, s, mon.pal[idx]);
      }
  }

  uint32_t ht = millis() - hitTime;
  if (hitTime && ht < 260) {
    int rad = 22 + (int)(ht / 6);
    gfx->drawCircle((int)hitX, (int)hitY, rad, ballColor565(0xff, 0xe7, 0x9f));
    gfx->drawCircle((int)hitX, (int)hitY, rad - 2, ballColor565(0xff, 0xd9, 0x8a));
  }

  drawMap(SPR_ICON_PLAY, 16, (int)ballX - 24, (int)ballY - 24, 3, false);
  flushStampedFrame();
}
