#include "game_clean_screen.h"
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

extern void drawGameScene();
extern void flushStampedFrame();
extern int sceneHour();

GameCleanScreen gameCleanScreen;

static constexpr int CLEAN_CX = 233;

/** Converts 8-bit RGB components to an RGB565 display color. */
static uint16_t cleanColor565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
}

void GameCleanScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) { closeGame(); sfxPlay(SFX_TAP); return; }
  for (uint8_t i = 0; i < 4; i++) {
    if (!cleanAlive[i]) continue;
    int dx = x - cleanX[i], dy = y - cleanY[i];
    if (dx * dx + dy * dy <= 38 * 38) {
      cleanAlive[i] = false;
      if (cleanActive) cleanActive--;
      gameScore++;
      hitX = cleanX[i];
      hitY = cleanY[i];
      hitTime = millis();
      sfxPlay(SFX_MINIGAME_OK);
      return;
    }
  }
  if (++gameMisses >= 3) finishGame();
  else sfxPlay(SFX_MINIGAME_BAD);
}

void GameCleanScreen::render() {
  uint32_t now = millis();
  if (gameOverUntil) {
    renderResult(T(S_RECORD_FMT), pet.cleanHi, S_HYG_GAIN_FMT);
    return;
  }
  if (deadlineReached(now, cleanUntil) || gameMisses >= 3) {
    finishGame();
    return;
  }
  if (deadlineReached(now, cleanSpawnAt)) {
    if (cleanActive < 4) spawnSpot();
    cleanSpawnAt = now + 720 - (gameScore > 12 ? 260 : gameScore * 20);
  }
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  gfx->setTextColor(ink);
  gfx->setTextSize(3);
  gfx->setCursor(CLEAN_CX - strlen(T(S_CLEAN_TITLE)) * 9, 32);
  gfx->print(T(S_CLEAN_TITLE));
  char score[16], rec[16];
  snprintf(score, sizeof(score), T(S_SCORE_FMT), gameScore);
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.cleanHi);
  gfx->setTextSize(2);
  gfx->setCursor(50, 78);
  gfx->print(score);
  gfx->setCursor(294, 78);
  gfx->print(rec);
  for (int i = 0; i < 3; i++) {
    if (i < 3 - gameMisses) gfx->fillCircle(180 + i * 28, 104, 6, UI_BAR_BAD);
    else gfx->drawCircle(180 + i * 28, 104, 6, UI_TRACK);
  }
  for (uint8_t i = 0; i < 4; i++) {
    if (!cleanAlive[i]) continue;
    gfx->fillCircle(cleanX[i], cleanY[i], 26, cleanColor565(0x8a, 0x66, 0x45));
    gfx->drawCircle(cleanX[i], cleanY[i], 28, UI_INK);
    gfx->fillCircle(cleanX[i] - 8, cleanY[i] - 8, 5, cleanColor565(0x62, 0x45, 0x2e));
    gfx->fillCircle(cleanX[i] + 10, cleanY[i] + 4, 6, cleanColor565(0x62, 0x45, 0x2e));
  }
  int bw = 280;
  int fw = (int)((uint32_t)bw * (cleanUntil - now) / 18000);
  if (fw < 0) fw = 0;
  gfx->fillRoundRect(CLEAN_CX - bw / 2, 362, bw, 16, 5, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(CLEAN_CX - bw / 2, 362, fw, 16, 5, UI_BAR_OK);
  uint32_t ht = millis() - hitTime;
  if (hitTime && ht < 220) gfx->drawCircle((int)hitX, (int)hitY, 42 + ht / 8, UI_BAR_OK);
  flushStampedFrame();
}

void GameCleanScreen::spawnSpot() {
  for (uint8_t i = 0; i < 4; i++) {
    if (cleanAlive[i]) continue;
    cleanX[i] = 88 + random(290);
    cleanY[i] = 122 + random(224);
    cleanAlive[i] = true;
    cleanActive++;
    return;
  }
}

void GameCleanScreen::start() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  resetGame();
  cleanActive = 0;
  for (uint8_t i = 0; i < 4; i++) cleanAlive[i] = false;
  cleanUntil = millis() + 18000;
  cleanSpawnAt = millis();
  spawnSpot();
  setScreen(&gameCleanScreen);
  sfxPlay(SFX_GAME_START);
}

void GameCleanScreen::finishGame() {
  gameNewHi = (gameScore > pet.cleanHi);
  gameGain = pet.applyCleanResult(gameScore);
  sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  showResult();
}
