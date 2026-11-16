#include "settings_screen.h"

#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>

#include "../../audio.h"
#include "../../i18n.h"
#include "../../species.h"
#include "clock_screen.h"
#include "game_info_screen.h"
#include "power_manager.h"
#include "settings_logic.h"

extern Arduino_Canvas *gfx;
extern void flushStampedFrame();

SettingsScreen settingsScreen;

struct SettingsLabels {
  const char *title;
  const char *brightness;
  const char *volume;
  const char *power;
  const char *system;
  const char *dateTime;
  const char *info;
  const char *preview;
  const char *medium;
  const char *maximum;
};

static const SettingsLabels LABELS[LANG_COUNT] = {
  { "AJUSTES", "BRILLO", "VOLUMEN", "AHORRO", "SISTEMA", "FECHA/HORA", "DATOS", "BRILLO/WI-FI: VISTA PREVIA", "MEDIO", "MAX" },
  { "SETTINGS", "BRIGHTNESS", "VOLUME", "POWER SAVE", "SYSTEM", "DATE/TIME", "GAME INFO", "BRIGHTNESS/WI-FI: PREVIEW", "MED", "MAX" },
  { "REGLAGES", "LUMINOSITE", "VOLUME", "ECONOMIE", "SYSTEME", "DATE/HEURE", "INFOS", "LUMIERE/WI-FI: APERCU", "MOYEN", "MAX" },
  { "EINSTELLUNG", "HELLIGKEIT", "LAUTSTAERKE", "ENERGIESPAREN", "SYSTEM", "DATUM/ZEIT", "INFOS", "HELLIGKEIT/WI-FI: VORSCHAU", "MITTEL", "MAX" },
  { "IMPOSTAZIONI", "LUMINOSITA", "VOLUME", "RISPARMIO", "SISTEMA", "DATA/ORA", "INFO", "LUCE/WI-FI: ANTEPRIMA", "MEDIO", "MAX" },
  { "CONFIGURACAO", "BRILHO", "VOLUME", "ECONOMIA", "SISTEMA", "DATA/HORA", "DADOS", "BRILHO/WI-FI: PREVIA", "MEDIO", "MAX" },
};

static const SettingsLabels &labels() {
  return LABELS[gLang < LANG_COUNT ? (uint8_t)gLang : (uint8_t)LANG_EN];
}

static void centered(const char *text, int x, int y, int size, uint16_t color) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(x - (int)strlen(text) * 3 * size, y);
  gfx->print(text);
}

static void drawSlider(int y, const char *label, const char *value, int percent) {
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(76, y);
  gfx->print(label);
  gfx->setCursor(390 - (int)strlen(value) * 12, y);
  gfx->print(value);
  gfx->fillRoundRect(76, y + 23, 314, 18, 9, UI_WHITE);
  gfx->drawRoundRect(76, y + 23, 314, 18, 9, UI_INK);
  if (percent > 0) gfx->fillRoundRect(78, y + 25, 310 * percent / 100, 14, 7, UI_BAR_OK);
  gfx->fillCircle(82 + 302 * percent / 100, y + 32, 10, UI_INK);
  centered("-", 61, y + 25, 2, UI_TRACK);
  centered("+", 405, y + 25, 2, UI_TRACK);
}

static void drawToggle(int y, const char *label, bool on) {
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(76, y + 9);
  gfx->print(label);
  gfx->fillRoundRect(312, y, 78, 34, 17, on ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(312, y, 78, 34, 17, UI_INK);
  gfx->fillCircle(on ? 373 : 329, y + 17, 12, UI_INK);
}

void SettingsScreen::open() {
  page = 0;
  setScreen(this);
  sfxPlay(SFX_MENU);
}

void SettingsScreen::render() {
  dirty = false;
  const SettingsLabels &l = labels();
  gfx->fillScreen(UI_BG_DAY);
  centered(l.title, 233, 42, 3, UI_INK);
  if (page == 0) {
    char brightnessText[6];
    snprintf(brightnessText, sizeof(brightnessText), "%u%%", brightness);
    uint8_t volume = settingsVolumeLevel(audioMode());
    const char *volumeText = audioMode() == SOUND_LOW ? "LOW" :
                             volume == 0 ? "OFF" : volume == 1 ? l.medium : l.maximum;
    drawSlider(100, l.brightness, brightnessText, brightness);
    drawSlider(169, l.volume, volumeText, volume * 50);
    drawToggle(247, "WI-FI", wifi);
    drawToggle(305, l.power, powerSave);
    centered(l.preview, 233, 367, 1, UI_TRACK);
    centered("1/2   >", 233, 391, 2, UI_TRACK);
  } else {
    const char *items[] = { l.system, l.dateTime, "WI-FI", l.info };
    for (int i = 0; i < 4; ++i) {
      int x = 77 + (i % 2) * 164;
      int y = i < 2 ? 96 : 240;
      bool enabled = i == 1 || i == 3;
      uint16_t ink = enabled ? UI_INK : UI_TRACK;
      gfx->fillRoundRect(x, y, 148, 128, 16, enabled ? UI_WHITE : 0xE73D);
      gfx->drawRoundRect(x, y, 148, 128, 16, ink);
      if (enabled) gfx->drawRoundRect(x + 1, y + 1, 146, 126, 15, ink);
      int cx = x + 74;
      int cy = y + 47;
      if (i == 0) {
        gfx->drawRoundRect(cx - 24, cy - 19, 48, 38, 5, ink);
        gfx->drawFastHLine(cx - 18, cy - 8, 36, ink);
        gfx->drawFastHLine(cx - 18, cy + 2, 36, ink);
      } else if (i == 1) {
        gfx->drawCircle(cx, cy, 23, ink);
        gfx->drawLine(cx, cy, cx, cy - 14, ink);
        gfx->drawLine(cx, cy, cx + 12, cy + 6, ink);
      } else if (i == 3) {
        gfx->fillCircle(cx, cy - 15, 4, ink);
        gfx->fillRoundRect(cx - 4, cy - 6, 8, 28, 3, ink);
      } else {
        gfx->drawLine(cx - 24, cy - 8, cx, cy - 22, ink);
        gfx->drawLine(cx, cy - 22, cx + 24, cy - 8, ink);
        gfx->drawLine(cx - 14, cy + 2, cx, cy - 6, ink);
        gfx->drawLine(cx, cy - 6, cx + 14, cy + 2, ink);
        gfx->fillCircle(cx, cy + 13, 4, ink);
      }
      centered(items[i], cx, y + 94, strlen(items[i]) > 12 ? 1 : 2, ink);
    }
    centered("<   2/2", 233, 391, 2, UI_TRACK);
  }
  centered("SWIPE UP TO EXIT", 233, 420, 1, UI_TRACK);
  flushStampedFrame();
}

void SettingsScreen::handleTap(int16_t x, int16_t y) {
  if (y >= 418) {
    setScreen(nullptr);
    sfxPlay(SFX_TAP);
    return;
  }
  if (page == 1) {
    if (x >= 241 && x < 389 && y >= 96 && y < 224) clockSettingsScreen.open(this);
    else if (x >= 241 && x < 389 && y >= 240 && y < 368) gameInfoScreen.open(this);
    return;
  }
  if (x < 48 || x >= 418) return;
  int direction = x < 233 ? -1 : 1;
  if (y >= 100 && y < 151) {
    brightness = settingsBrightnessStep(brightness, direction);
  } else if (y >= 169 && y < 220) {
    uint8_t mode = settingsVolumeMode(audioMode(), direction);
    if (mode != audioMode()) audioSetMode(mode);
    if (audioEnabled()) sfxPlay(SFX_LEVEL);
  } else if (y >= 247 && y < 281) {
    wifi = !wifi;
  } else if (y >= 305 && y < 339) {
    setPowerSave(!powerSave);
    sfxPlay(powerSave ? SFX_MENU : SFX_TAP);
  } else {
    return;
  }
  dirty = true;
}

void SettingsScreen::handleSwipe(int direction) {
  if ((direction > 0 && page == 0) || (direction < 0 && page == 1)) {
    page = direction > 0 ? 1 : 0;
    dirty = true;
    sfxPlay(SFX_MENU);
  }
}

void SettingsScreen::handleSwipeV(int) {
  setScreen(nullptr);
  sfxPlay(SFX_TAP);
}
