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

extern void drawGameScene();
extern void flushStampedFrame();
extern int sceneHour();
extern const char *battleTypeName(uint8_t type);
extern uint16_t battleTypeColor(uint8_t type);
extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);

GameTypeScreen gameTypeScreen;

static const uint8_t TYPE_DEF_POOL[] = {
  TYPE_GRASS, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_ROCK, TYPE_GROUND,
  TYPE_FLYING, TYPE_POISON, TYPE_PSYCHIC, TYPE_GHOST, TYPE_DRAGON, TYPE_ICE
};
static const uint8_t TYPE_COUNTER_POOL[] = {
  TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_GROUND, TYPE_WATER, TYPE_WATER,
  TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_BUG, TYPE_GHOST, TYPE_ICE, TYPE_FIRE
};
static const uint8_t TYPE_OPTION_POOL[] = {
  TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_GRASS, TYPE_ICE,
  TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_FLYING, TYPE_PSYCHIC, TYPE_BUG,
  TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON
};


static constexpr int TYPE_CX = 233;

void GameTypeScreen::handleTap(int16_t x, int16_t y) {
  if (gameOverUntil) return;
  if (y < 72) { closeGame(); sfxPlay(SFX_TAP); return; }
  int idx = -1;
  for (int i = 0; i < 3; i++) {
    int by = 210 + i * 60;
    if (x >= 70 && x <= 396 && y >= by - 8 && y <= by + 56) idx = i;
  }
  if (idx < 0) return;
  if ((uint8_t)idx == typeCorrect) {
    gameScore++;
    sfxPlay(SFX_MINIGAME_OK);
    nextQuestion();
  } else {
    if (++gameMisses >= 3) finishGame();
    else sfxPlay(SFX_MINIGAME_BAD);
  }
}

void GameTypeScreen::render() {
  uint32_t now = millis();
  if (gameOverUntil) {
    renderResult(T(S_RECORD_FMT), pet.typeHi, S_ATK_GAIN_FMT);
    return;
  }
  if (deadlineReached(now, typeUntil)) {
    if (++gameMisses >= 3) {
      finishGame();
      return;
    }
    sfxPlay(SFX_MINIGAME_BAD);
    nextQuestion();
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

void GameTypeScreen::nextQuestion() {
  uint8_t q = (uint8_t)random(sizeof(TYPE_DEF_POOL));
  typeEnemy = TYPE_DEF_POOL[q];
  uint8_t correct = TYPE_COUNTER_POOL[q];
  typeCorrect = (uint8_t)random(3);
  for (uint8_t i = 0; i < 3; i++) typeChoice[i] = TYPE_NONE;
  typeChoice[typeCorrect] = correct;
  for (uint8_t i = 0; i < 3; i++) {
    if (i == typeCorrect) continue;
    uint8_t cand;
    do {
      cand = TYPE_OPTION_POOL[random(sizeof(TYPE_OPTION_POOL))];
    } while (cand == correct || cand == typeChoice[0] || cand == typeChoice[1] || cand == typeChoice[2] ||
             battleTypeEffectPct(cand, typeEnemy, TYPE_NONE) > 100);
    typeChoice[i] = cand;
  }
  typeUntil = millis() + 4200;
}

void GameTypeScreen::start() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  resetGame();
  nextQuestion();
  setScreen(&gameTypeScreen);
  sfxPlay(SFX_GAME_START);
}

void GameTypeScreen::finishGame() {
  gameNewHi = (gameScore > pet.typeHi);
  gameGain = pet.applyTypeResult(gameScore);
  sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  showResult();
}
