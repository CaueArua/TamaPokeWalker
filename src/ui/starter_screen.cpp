#include "starter_screen.h"
#include <Arduino_GFX_Library.h>
#include <string.h>
#include "../../audio.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../species.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern void flushStampedFrame();
extern void lockTouchBrief(uint16_t ms = 160);
extern void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);
extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);
extern const char *dexName(int16_t dex);

static constexpr int16_t STARTER_DEX[3][3] = {
  { 1, 4, 7 },
  { 47, 50, 53 },
  { 69, 72, 75 },
};

static constexpr StrId STARTER_REGION_LABELS[3] = {
  S_RANK_KANTO, S_RANK_JOHTO, S_RANK_HOENN
};

static constexpr int STARTER_CX = 233;
static constexpr int STARTER_ROW_Y = 110;
static constexpr int STARTER_ROW_H = 70;
static constexpr int STARTER_ROW_GAP = 8;

StarterScreen starterScreen;

void StarterScreen::onEnter() {
  region = 0xFF;
  dirty = true;
}

void StarterScreen::render() {
  dirty = false;
  gfx->fillScreen(UI_BG_DAY);

  if (region == 0xFF) {
    const char *title = T(S_CHOOSE_REGION);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(STARTER_CX - strlen(title) * 6, 68);
    gfx->print(title);
    for (uint8_t i = 0; i < 3; i++) {
      int rowY = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      gfx->fillRoundRect(70, rowY, 326, STARTER_ROW_H, 14,
                         lerp565(UI_BAR_OK, UI_WHITE, 6, 8));
      gfx->drawRoundRect(70, rowY, 326, STARTER_ROW_H, 14, UI_BAR_OK);
      gfx->setTextColor(UI_INK);
      gfx->setTextSize(3);
      const char *label = T(STARTER_REGION_LABELS[i]);
      gfx->setCursor(STARTER_CX - strlen(label) * 9, rowY + 24);
      gfx->print(label);
    }
  } else {
    const char *title = T(S_CHOOSE_STARTER);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(STARTER_CX - strlen(title) * 6, 68);
    gfx->print(title);
    for (int i = 0; i < 3; i++) {
      int16_t dex = STARTER_DEX[region][i];
      const DexEntry &entry = DEX_TBL[dex];
      int rowY = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      gfx->fillRoundRect(70, rowY, 326, STARTER_ROW_H, 14,
                         lerp565(entry.accent, UI_WHITE, 6, 8));
      gfx->drawRoundRect(70, rowY, 326, STARTER_ROW_H, 14, entry.accent);
      const uint8_t *thumbnail = thumbs.get(dex);
      if (thumbnail) drawThumb(thumbnail, 76, rowY - 5, 3, false);
      gfx->setTextColor(UI_INK);
      gfx->setTextSize(3);
      gfx->setCursor(178, rowY + 24);
      gfx->print(dexName(dex));
    }
  }
  flushStampedFrame();
}

void StarterScreen::handleTap(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 3; i++) {
    int rowY = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
    if (x < 70 || x > 396 || y < rowY || y > rowY + STARTER_ROW_H) continue;

    if (region == 0xFF) {
      region = i;
      dirty = true;
      lockTouchBrief();
      sfxPlay(SFX_TAP);
      return;
    }

    pet.chooseStarter(STARTER_DEX[region][i]);
    region = 0xFF;
    setScreen(nullptr);
    sfxPlay(SFX_TAP);
    return;
  }
}
