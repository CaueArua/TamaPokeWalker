#include "strength_screen.h"
#include <Arduino_GFX_Library.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../i18n.h"
#include "../../species.h"
#include "../../time_utils.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern uint32_t lastInteract;
extern int sceneHour();
extern void drawGameScene();
extern void flushStampedFrame();

static constexpr int SACK_CX = 233;

static uint16_t sackColor565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
}

StrengthTrainingScreen strengthTrainingScreen;

bool StrengthTrainingScreen::isOpen() const {
  return activeScreen == this;
}

void StrengthTrainingScreen::open() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  setScreen(this);
}

void StrengthTrainingScreen::onEnter() {
  sackUntil = millis() + 10000;
  sackOverUntil = 0;
  sackHits = 0;
  sackShake = 0;
  sackNewHi = false;
  sfxPlay(SFX_GAME_START);
}

void StrengthTrainingScreen::handlePress(int16_t y) {
  lastInteract = millis();
  if (y < 72) {
    setScreen(nullptr);
    return;
  }
  if (deadlineReached(millis(), sackUntil)) return;
  sackHits++;
  sackShake = 16;
  if ((sackHits & 1) == 1) sfxPlay(SFX_PLAY);
}

void StrengthTrainingScreen::render() {
  uint32_t now = millis();
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;

  if (sackOverUntil) {
    if (deadlineReached(now, sackOverUntil)) {
      setScreen(nullptr);
      return;
    }
    char b[20];
    snprintf(b, sizeof(b), T(S_HITS_FMT), sackHits);
    gfx->setTextColor(ink);
    gfx->setTextSize(4);
    gfx->setCursor(SACK_CX - strlen(b) * 12, 150);
    gfx->print(b);
    char g[18];
    snprintf(g, sizeof(g), T(S_STR_GAIN_FMT), sackGain);
    gfx->setTextColor(UI_BAR_BAD);
    gfx->setTextSize(3);
    gfx->setCursor(SACK_CX - strlen(g) * 9, 210);
    gfx->print(g);
    gfx->setTextSize(2);
    if (sackNewHi && sackHits > 0) {
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setCursor(SACK_CX - strlen(T(S_NEW_RECORD)) * 6, 256);
      gfx->print(T(S_NEW_RECORD));
    } else {
      char r[18];
      snprintf(r, sizeof(r), T(S_RECORD_FMT), pet.strHi);
      gfx->setTextColor(ink);
      gfx->setCursor(SACK_CX - strlen(r) * 6, 256);
      gfx->print(r);
    }
    flushStampedFrame();
    return;
  }

  if (deadlineReached(now, sackUntil)) {
    sackNewHi = (sackHits > pet.strHi);
    sackGain = pet.trainStrength(sackHits);
    sfxPlay(sackNewHi ? SFX_MEDAL : SFX_PLAY);
    sackOverUntil = now + 3500;
    flushStampedFrame();
    return;
  }

  sackShake *= 0.84f;
  int off = (int)(sackShake * sinf(now * 0.05f));
  int sx = SACK_CX + off, top = 86, sy = 150;
  gfx->fillRect(SACK_CX - 3, 56, 6, top - 56, ink);
  gfx->fillRect(sx - 4, top - 30, 8, 34, ink);
  gfx->fillRoundRect(sx - 42, top, 84, 150, 26, sackColor565(0xb5, 0x3a, 0x3a));
  gfx->fillRoundRect(sx - 42, top, 84, 22, 18, sackColor565(0x7e, 0x28, 0x28));
  gfx->drawRoundRect(sx - 42, top, 84, 150, 26, ink);
  gfx->fillRect(sx - 42, top + 70, 84, 4, sackColor565(0x7e, 0x28, 0x28));

  char buf[8];
  snprintf(buf, sizeof(buf), "%u", sackHits);
  gfx->setTextColor(ink);
  gfx->setTextSize(6);
  gfx->setCursor(SACK_CX - strlen(buf) * 18, 268);
  gfx->print(buf);

  gfx->setTextSize(2);
  gfx->setCursor(SACK_CX - strlen(T(S_HIT_FAST)) * 6, 322);
  gfx->print(T(S_HIT_FAST));

  uint32_t left = sackUntil - now;
  int bw = 280, fw = (int)((uint32_t)bw * left / 10000);
  gfx->fillRoundRect(SACK_CX - bw / 2, 350, bw, 16, 5, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(SACK_CX - bw / 2, 350, fw, 16, 5, UI_BAR_OK);

  flushStampedFrame();
}
