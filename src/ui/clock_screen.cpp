#include "clock_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../rtcbat.h"
#include "../../i18n.h"
#include "../../audio.h"
#include "help_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon pmd;
extern SdMon mon;
extern bool clockOpen;
extern bool clockDirty;
extern bool powerSave;
extern int clockH;
extern int clockM;
extern void setPowerSave(bool on);
extern void openHelp();
extern void flushStampedFrame();
extern const char *firmwareVersion();

ClockSettingsScreen clockSettingsScreen;
static constexpr int SCREEN_CX = 233;

void ClockSettingsScreen::applyTime() {
  uint32_t base = pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch();
  uint32_t e = (base / 86400) * 86400 + (uint32_t)clockH * 3600 + (uint32_t)clockM * 60;
  rtcSetEpoch(e);
  pet.setClock(e);
  clockOpen = false;
  setScreen(nullptr);
}

void drawClockBtn(int x, int y, const char *l) {
  gfx->fillRoundRect(x, y, 58, 58, 12, UI_WHITE);
  gfx->drawRoundRect(x, y, 58, 58, 12, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(4);
  gfx->setCursor(x + 17, y + 15);
  gfx->print(l);
}

// pildoras de idioma centradas en y; rellena la activa
#define LANG_PILL_Y 296
#define LANG_PILL_H 30
#define LANG_PILL_X 336          // pildora de idioma (cicla los 6 al tocar)
#define LANG_PILL_W 96
#define SOUND_PILL_X 24
#define SOUND_PILL_W 116
#define PSAVE_PILL_X 150
#define PSAVE_PILL_W 176

const char *soundModeLabel() {
  switch (audioMode()) {
    case SOUND_FULL: return T(S_SND_FULL);
    case SOUND_MED: return T(S_SND_MED);
    case SOUND_LOW: return T(S_SND_LOW);
    default: return T(S_SND_OFF);
  }
}

uint8_t nextSoundMode() {
  switch (audioMode()) {
    case SOUND_FULL: return SOUND_MED;
    case SOUND_MED: return SOUND_LOW;
    case SOUND_LOW: return SOUND_OFF;
    default: return SOUND_FULL;
  }
}
static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT" };

const char *powerSaveLabel() {
  return powerSave ? T(S_PSAVE_ON) : T(S_PSAVE_OFF);
}

void drawStatusLine(int y, const char *label, const char *value, uint16_t valueColor) {
  gfx->setTextSize(1);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(94, y);
  gfx->print(label);
  gfx->setTextColor(valueColor);
  gfx->setCursor(172, y);
  gfx->print(value);
}

void ClockSettingsScreen::render() {
  clockDirty = false;
  gfx->fillScreen(UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(SCREEN_CX - strlen(T(S_SET_TIME)) * 9, 44);
  gfx->print(T(S_SET_TIME));

  char t[8];
  snprintf(t, sizeof(t), "%02d:%02d", clockH, clockM);
  gfx->setTextSize(7);
  gfx->setCursor(SCREEN_CX - 105, 108);
  gfx->print(t);

  drawClockBtn(104, 190, "-");  // hora -
  drawClockBtn(170, 190, "+");  // hora +
  drawClockBtn(252, 190, "-");  // min -
  drawClockBtn(318, 190, "+");  // min +
  gfx->setTextSize(2);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(120, 256);
  gfx->print(T(S_HOUR));
  gfx->setCursor(276, 256);
  gfx->print(T(S_MIN));

  // selector de sonido: mucho / medio / poco / apagado
  uint8_t sndMode = audioMode();
  const char *sl = soundModeLabel();
  gfx->fillRoundRect(SOUND_PILL_X, LANG_PILL_Y, SOUND_PILL_W, LANG_PILL_H, 8, sndMode ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(SOUND_PILL_X, LANG_PILL_Y, SOUND_PILL_W, LANG_PILL_H, 8, UI_INK);
  gfx->setTextColor(sndMode ? UI_BG_DAY : UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(SOUND_PILL_X + (SOUND_PILL_W - (int)strlen(sl) * 12) / 2, LANG_PILL_Y + 8);
  gfx->print(sl);

  // selector de ahorro opcional: apagado por defecto, persistente
  const char *pl = powerSaveLabel();
  gfx->fillRoundRect(PSAVE_PILL_X, LANG_PILL_Y, PSAVE_PILL_W, LANG_PILL_H, 8, powerSave ? UI_BAR_WARN : UI_WHITE);
  gfx->drawRoundRect(PSAVE_PILL_X, LANG_PILL_Y, PSAVE_PILL_W, LANG_PILL_H, 8, UI_INK);
  gfx->setTextColor(powerSave ? UI_BG_DAY : UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PSAVE_PILL_X + (PSAVE_PILL_W - (int)strlen(pl) * 12) / 2, LANG_PILL_Y + 8);
  gfx->print(pl);

  // selector de idioma: una pildora que cicla los 6 idiomas al tocar
  gfx->fillRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, UI_WHITE);
  gfx->drawRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, UI_INK);
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", LANG_CODES[gLang]);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(LANG_PILL_X + (LANG_PILL_W - (int)strlen(lp) * 12) / 2, LANG_PILL_Y + 8);
  gfx->print(lp);

  char fwLine[34];
  snprintf(fwLine, sizeof(fwLine), "v%s", firmwareVersion());
  drawStatusLine(334, "FW", fwLine, UI_INK);
  drawStatusLine(348, "SAVE", pet.saveLoadedFromNvs ? T(S_OK) : T(S_STATUS_NEW), pet.saveLoadedFromNvs ? UI_BAR_OK : UI_BAR_WARN);
  drawStatusLine(362, "SD", sdReady ? "OK" : "NO", sdReady ? UI_BAR_OK : UI_BAR_WARN);
  drawStatusLine(376, "SPR", (pmd.loaded || mon.loaded) ? T(S_OK) : T(S_STATUS_FLASH_NO), (pmd.loaded || mon.loaded) ? UI_BAR_OK : UI_BAR_WARN);
  char petLine[32];
  if (pet.isEgg()) snprintf(petLine, sizeof(petLine), "%s", T(S_EGG_HDR));
  else {
    char level[12];
    snprintf(level, sizeof(level), T(S_LEVEL_SHORT_FMT), pet.level());
    snprintf(petLine, sizeof(petLine), "#%d %s", pet.speciesId, level);
  }
  drawStatusLine(390, "PET", petLine, UI_INK);

  gfx->fillRoundRect(96, 404, 110, 40, 13, UI_WHITE);
  gfx->drawRoundRect(96, 404, 110, 40, 13, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  const char *hw = helpLabelForLanguage((uint8_t)gLang);
  gfx->setCursor(96 + (110 - (int)strlen(hw) * 12) / 2, 417);
  gfx->print(hw);

  gfx->fillRoundRect(230, 404, 156, 40, 13, UI_BAR_OK);
  gfx->setTextColor(UI_BG_DAY);
  gfx->setTextSize(3);
  gfx->setCursor(230 + (156 - 36) / 2, 414);
  gfx->print(T(S_OK));
  flushStampedFrame();
}

void ClockSettingsScreen::handleTap(int16_t x, int16_t y) {
  if (y >= 190 && y <= 248) {  // fila de botones +/-
    if (x >= 104 && x < 162) clockH = (clockH + 23) % 24;
    else if (x >= 170 && x < 228) clockH = (clockH + 1) % 24;
    else if (x >= 252 && x < 310) clockM = (clockM + 59) % 60;
    else if (x >= 318 && x < 376) clockM = (clockM + 1) % 60;
    clockDirty = true;
    return;
  }
  if (y >= LANG_PILL_Y && y <= LANG_PILL_Y + LANG_PILL_H) {
    if (x >= SOUND_PILL_X && x < SOUND_PILL_X + SOUND_PILL_W) {
      audioSetMode(nextSoundMode());
      clockDirty = true;
      if (audioEnabled()) sfxPlay(SFX_LEVEL);  // confirma el nuevo modo si no esta apagado
      return;
    }
    if (x >= PSAVE_PILL_X && x < PSAVE_PILL_X + PSAVE_PILL_W) {
      setPowerSave(!powerSave);
      clockDirty = true;
      sfxPlay(powerSave ? SFX_MENU : SFX_TAP);
      return;
    }
    if (x >= LANG_PILL_X && x < LANG_PILL_X + LANG_PILL_W) {  // cicla idioma
      setLang((Lang)((gLang + 1) % LANG_COUNT));
      clockDirty = true;
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (y >= 404 && y <= 444 && x >= 96 && x <= 206) { openHelp(); return; }
  if (y >= 404 && y <= 444 && x >= 230 && x <= 386) { applyTime(); return; }
}

