#include "activity_menu_screen.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <string.h>

#include "../../audio.h"
#include "../../dex.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../species.h"
#include "battle_screen.h"
#include "pet_card_expedition_screen.h"
#include "pet_card_page.h"
#include "render_utils.h"
#include "screen_controller.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern void flushStampedFrame();

static constexpr int MENU_CX = 233;
static constexpr int BTN_W = 148;
static constexpr int BTN_H = 128;
static constexpr int BTN_GAP_X = 16;
static constexpr int BTN_GAP_Y = 16;
static constexpr int GRID_X0 = MENU_CX - BTN_W - BTN_GAP_X / 2;
static constexpr int GRID_Y0 = 96;

static constexpr uint16_t SUMMARY_BG = 0xC6FF;
static constexpr uint16_t SUMMARY_ACCENT = 0x3D7F;
static constexpr uint16_t TRAINING_BG = 0xFED3;
static constexpr uint16_t TRAINING_ACCENT = 0xB44A;
static constexpr uint16_t WILD_BG = 0xC7F1;
static constexpr uint16_t WILD_ACCENT = 0x36E4;
static constexpr uint16_t EXPEDITION_BG = 0xD6B2;
static constexpr uint16_t EXPEDITION_ACCENT = 0x6A29;

ActivityMenuScreen activityMenuScreen;

struct ActivityMenuItem {
  const char *label;
  uint16_t background;
  uint16_t accent;
};

static const ActivityMenuItem MENU_ITEMS[] = {
  { "SUMMARY", SUMMARY_BG, SUMMARY_ACCENT },
  { "TRAINING", TRAINING_BG, TRAINING_ACCENT },
  { "WILD BATTLE", WILD_BG, WILD_ACCENT },
  { "EXPEDITION", EXPEDITION_BG, EXPEDITION_ACCENT },
};

static void drawSummaryIcon(int cx, int cy) {
  if (!pet.isEgg() && pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    const uint8_t *thumbnail = thumbs.get(pet.speciesId);
    if (thumbnail) {
      drawThumb(thumbnail, cx - 37, cy - 37, 3, false);
      return;
    }
  }

  gfx->fillRoundRect(cx - 18, cy - 24, 36, 48, 18, UI_WHITE);
  gfx->drawRoundRect(cx - 18, cy - 24, 36, 48, 18, SUMMARY_ACCENT);
  gfx->drawFastHLine(cx - 14, cy, 28, SUMMARY_ACCENT);
  gfx->fillCircle(cx, cy, 4, SUMMARY_ACCENT);
}

static void drawTrainingIcon(int cx, int cy, uint16_t color) {
  gfx->fillRoundRect(cx - 20, cy - 4, 40, 8, 4, color);
  gfx->fillRoundRect(cx - 33, cy - 17, 11, 34, 3, color);
  gfx->fillRoundRect(cx - 23, cy - 12, 8, 24, 3, color);
  gfx->fillRoundRect(cx + 15, cy - 12, 8, 24, 3, color);
  gfx->fillRoundRect(cx + 22, cy - 17, 11, 34, 3, color);
}

static void drawWildBattleIcon(int cx, int cy, uint16_t color) {
  gfx->fillRect(cx - 36, cy + 18, 72, 3, color);
  gfx->fillTriangle(cx - 30, cy + 18, cx - 20, cy - 18, cx - 10, cy + 18, color);
  gfx->fillTriangle(cx - 13, cy + 18, cx, cy - 28, cx + 13, cy + 18, color);
  gfx->fillTriangle(cx + 8, cy + 18, cx + 20, cy - 18, cx + 32, cy + 18, color);
  gfx->drawFastVLine(cx - 20, cy - 18, 14, UI_INK);
  gfx->drawFastVLine(cx, cy - 28, 17, UI_INK);
  gfx->drawFastVLine(cx + 20, cy - 18, 14, UI_INK);
}

static void drawExpeditionIcon(int cx, int cy, uint16_t color) {
  gfx->drawLine(cx - 30, cy + 22, cx - 12, cy + 4, color);
  gfx->drawLine(cx - 12, cy + 4, cx + 8, cy + 12, color);
  gfx->drawLine(cx + 8, cy + 12, cx + 30, cy - 18, color);
  gfx->fillCircle(cx - 27, cy + 19, 3, color);
  gfx->fillCircle(cx - 12, cy + 4, 3, color);
  gfx->fillCircle(cx + 8, cy + 12, 3, color);
  gfx->fillCircle(cx + 30, cy - 18, 3, color);
  gfx->fillTriangle(cx - 26, cy - 5, cx - 15, cy - 24, cx - 4, cy - 5, 0x4D6A);
  gfx->fillTriangle(cx - 9, cy - 5, cx + 4, cy - 30, cx + 17, cy - 5, 0x4D6A);
}

static int8_t menuItemAt(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 4; i++) {
    int itemX = GRID_X0 + (i % 2) * (BTN_W + BTN_GAP_X);
    int itemY = GRID_Y0 + (i / 2) * (BTN_H + BTN_GAP_Y);
    if (x >= itemX && x < itemX + BTN_W &&
        y >= itemY && y < itemY + BTN_H) return (int8_t)i;
  }
  return -1;
}

void ActivityMenuScreen::render() {
  dirty = false;
  gfx->fillScreen(UI_BG_DAY);

  const char *title = "ACTIVITIES";
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(MENU_CX - (int)strlen(title) * 9, 42);
  gfx->print(title);

  for (uint8_t i = 0; i < 4; i++) {
    const ActivityMenuItem &item = MENU_ITEMS[i];
    int x = GRID_X0 + (i % 2) * (BTN_W + BTN_GAP_X);
    int y = GRID_Y0 + (i / 2) * (BTN_H + BTN_GAP_Y);
    int iconCx = x + BTN_W / 2;
    int iconCy = y + 48;

    gfx->fillRoundRect(x, y, BTN_W, BTN_H, 16, item.background);
    gfx->drawRoundRect(x, y, BTN_W, BTN_H, 16, item.accent);
    gfx->drawRoundRect(x + 1, y + 1, BTN_W - 2, BTN_H - 2, 15, item.accent);

    if (i == 0) drawSummaryIcon(iconCx, iconCy);
    else if (i == 1) drawTrainingIcon(iconCx, iconCy, item.accent);
    else if (i == 2) drawWildBattleIcon(iconCx, iconCy, item.accent);
    else drawExpeditionIcon(iconCx, iconCy, item.accent);

    uint8_t textSize = strlen(item.label) > 12 ? 1 : 2;
    int labelWidth = (int)strlen(item.label) * 6 * textSize;
    gfx->setTextSize(textSize);
    gfx->setTextColor(UI_INK);
    gfx->setCursor(x + (BTN_W - labelWidth) / 2, y + 94);
    gfx->print(item.label);
  }

  gfx->setTextSize(1);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(MENU_CX - 48, 396);
  gfx->print("SWIPE TO RETURN");
  flushStampedFrame();
}

void ActivityMenuScreen::handleTap(int16_t x, int16_t y) {
  if (y >= 385) {
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
    return;
  }

  switch (menuItemAt(x, y)) {
    case 0:
      openPetCardPage(0, this);
      sfxPlay(SFX_MENU);
      break;
    case 2:
      battleScreen.start();
      break;
    case 3:
      petCardExpeditionScreen.open(this);
      sfxPlay(SFX_MENU);
      break;
    default:
      break;
  }
}

void ActivityMenuScreen::handleSwipe(int direction) {
  if(direction == -1){
      sfxPlay(SFX_TAP);
      setScreen(nullptr);
  }
}
