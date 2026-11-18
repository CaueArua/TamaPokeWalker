#include "clock_screen.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>

#include "../../audio.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../species.h"
#include "../core/clock.h"
#include "../core/clock_service.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern void flushStampedFrame();

bool clockDirty = true;
ClockSettingsScreen clockSettingsScreen;
static ClockDateTime selectedDateTime;
static bool clockValid = false;
static bool clockUnset = false;  // RTC present but holding no valid time.
static bool clockTouched = false;
static uint32_t clockRenderedMs = 0;
// 2026-01-01 00:00 UTC, shown while the RTC has no valid time and no saved time exists.
static const uint32_t CLOCK_FALLBACK_EPOCH = 1767225600UL;

static const char *const TITLES[LANG_COUNT] = {
  "FECHA/HORA", "DATE/TIME", "DATE/HEURE", "DATUM/ZEIT", "DATA/ORA", "DATA/HORA"
};

static const char *const UNSET_LABELS[LANG_COUNT] = {
  "HORA NAO CONFIGURADA", "TIME NOT SET", "HEURE NON DEFINIE",
  "UHRZEIT NICHT GESETZT", "ORA NON IMPOSTATA", "HORA NO CONFIGURADA"
};

static void centered(const char *text, int x, int y, int size, uint16_t color) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(x - (int)strlen(text) * 3 * size, y);
  gfx->print(text);
}

/** Reloads the displayed date from the clock until the user starts editing. */
static void refreshFromClock() {
  if (clockTouched) return;
  uint32_t epoch = Clock::nowEpoch();
  clockValid = epoch != 0;
  clockUnset = !clockValid && Clock::available();
  if (!clockValid) epoch = pet.lastSeenEpoch ? pet.lastSeenEpoch : CLOCK_FALLBACK_EPOCH;
  selectedDateTime = clockDateTime(epoch);
  clockRenderedMs = millis();
}

void ClockSettingsScreen::openAtBoot(Screen *returnTo) {
  clockTouched = false;
  refreshFromClock();
  clockDirty = true;
  returnScreen = returnTo;
  setScreen(this);
}

void ClockSettingsScreen::open(Screen *returnTo) {
  openAtBoot(returnTo);
  sfxPlay(SFX_MENU);
}

void ClockSettingsScreen::applyTime() {
  ClockDateTime target = selectedDateTime;
  target.second = 0;
  uint32_t epoch = epochFromDateTime(target);
  Clock::set(epoch);
  pet.syncClock(epoch);
  closeSettings();
}

void ClockSettingsScreen::closeSettings() {
  Screen *target = returnScreen;
  returnScreen = nullptr;
  setScreen(target);
}

static void drawField(int x, int y, const char *label, int value, bool year) {
  char text[16];
  snprintf(text, sizeof(text), year ? "%04d" : "%02d", value);
  centered(label, x, y, 1, UI_TRACK);
  gfx->fillRoundRect(x - 26, y + 15, 52, 31, 8, UI_WHITE);
  gfx->drawRoundRect(x - 26, y + 15, 52, 31, 8, UI_INK);
  centered("+", x, y + 22, 2, UI_INK);
  centered(text, x, y + 53, 2, UI_INK);
  gfx->fillRoundRect(x - 26, y + 76, 52, 31, 8, UI_WHITE);
  gfx->drawRoundRect(x - 26, y + 76, 52, 31, 8, UI_INK);
  centered("-", x, y + 83, 2, UI_INK);
}

void ClockSettingsScreen::render() {
  clockDirty = false;
  refreshFromClock();
  gfx->fillScreen(UI_BG_DAY);
  uint8_t lang = gLang < LANG_COUNT ? (uint8_t)gLang : (uint8_t)LANG_EN;
  centered(TITLES[lang], 233, 42, 3, UI_INK);

  drawField(145, 85, "DD", selectedDateTime.day, false);
  drawField(233, 85, "MM", selectedDateTime.month, false);
  drawField(321, 85, "YYYY", selectedDateTime.year, true);
  drawField(180, 233, T(S_HOUR), selectedDateTime.hour, false);
  drawField(286, 233, T(S_MIN), selectedDateTime.minute, false);
  if (!clockValid) {
    if (clockUnset) centered(UNSET_LABELS[lang], 233, 205, 2, UI_BAR_BAD);
    else centered("RTC ERROR", 233, 205, 2, UI_BAR_BAD);
  }

  gfx->fillRoundRect(155, 399, 156, 42, 13, UI_BAR_OK);
  centered(T(S_OK), 233, 410, 3, UI_BG_DAY);
  flushStampedFrame();
}

void ClockSettingsScreen::handleTap(int16_t x, int16_t y) {
  clockTouched = true;
  if (x >= 155 && x < 311 && y >= 399 && y < 441) {
    applyTime();
    return;
  }
  int field = -1;
  int direction = 0;
  if ((y >= 100 && y < 131) || (y >= 161 && y < 192)) {
    direction = y < 131 ? 1 : -1;
    if (x >= 119 && x < 171) field = 0;
    else if (x >= 207 && x < 259) field = 1;
    else if (x >= 295 && x < 347) field = 2;
  } else if ((y >= 248 && y < 279) || (y >= 309 && y < 340)) {
    direction = y < 279 ? 1 : -1;
    if (x >= 154 && x < 206) field = 3;
    else if (x >= 260 && x < 312) field = 4;
  }
  if (field >= 0) {
    clockAdjustDateTime(selectedDateTime, field, direction);
    clockDirty = true;
    sfxPlay(SFX_TAP);
  }
}

void ClockSettingsScreen::handleSwipeV(int) {
  closeSettings();
  sfxPlay(SFX_TAP);
}

bool ClockSettingsScreen::isStaticClean() const {
  if (clockDirty) return false;
  return clockTouched || millis() - clockRenderedMs < 1000;
}

void ClockSettingsScreen::markDirty() {
  clockDirty = true;
}
