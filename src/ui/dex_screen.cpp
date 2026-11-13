#include "dex_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../dex.h"
#include "../../audio.h"
#include "../../i18n.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

extern const char *dexName(int16_t dex);
extern void typeText(char *buf, size_t cap, const DexEntry &d);
extern uint16_t battleTypeColor(uint8_t t);
extern void flushStampedFrame();
extern void lockTouchBrief(uint16_t ms = 160);
extern void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY,
                        uint32_t t, bool loop, bool sil, uint8_t maxS);

static constexpr int DEX_CX = 233;
static constexpr int GAL_X = 73;
static constexpr int GAL_Y = 116;
static constexpr int GAL_CELL = 74;

DexScreen dexScreen;

void DexScreen::openGrid() {
  opened = true;
  page = 0;
  detail = 0;
  dirty = true;
}

void DexScreen::openDetail(int16_t dex) {
  opened = true;
  detail = dex;
  dirty = true;
  galleryPmd.unload();
  if (dex >= 1 && dex <= DEX_COUNT) {
    galleryPmd.load(dex, pet.isShinyRegistered(dex));
  }
}

void DexScreen::onEnter() {
  opened = true;
  dirty = true;
}

void DexScreen::onExit() {
  opened = false;
  galleryPmd.unload();
}

bool DexScreen::dexVisible(int16_t dex) const {
  if (dex < 1 || dex > DEX_COUNT) return false;
  if (region == 1 && dex > 46) return false;
  if (region == 2 && (dex < 47 || dex > 68)) return false;
  if (region == 3 && dex < 69) return false;
  if (filter == 1) return pet.isRegistered(dex);
  if (filter == 2) return pet.isCaught(dex);
  if (filter == 3) return pet.isShinyRegistered(dex);
  return true;
}

uint16_t DexScreen::filteredCount() const {
  if (filter == 0) return DEX_COUNT;
  uint16_t count = 0;
  for (int16_t dex = 1; dex <= DEX_COUNT; dex++)
    if (dexVisible(dex)) count++;
  return count;
}

int DexScreen::pageCount() const {
  uint16_t count = filteredCount();
  int pages = (count + 15) / 16;
  return pages > 0 ? pages : 1;
}

int16_t DexScreen::dexAt(uint16_t index) const {
  for (int16_t dex = 1; dex <= DEX_COUNT; dex++) {
    if (!dexVisible(dex)) continue;
    if (index == 0) return dex;
    index--;
  }
  return 0;
}

void DexScreen::showDetail(int16_t dex) {
  detail = dex;
  galleryPmd.unload();
  galleryPmd.load(dex, pet.isShinyRegistered(dex));
}

void DexScreen::render() {
  if (detail) {
    gfx->fillScreen(UI_BG_DAY);
    const DexEntry &d = DEX_TBL[detail];
    bool reg = pet.isRegistered(detail);
    bool caught = pet.isCaught(detail);
    bool known = reg || caught;
    char head[24];
    snprintf(head, sizeof(head), "N.%03d %s%s", detail,
             pet.isShinyRegistered(detail) ? "*" : "", known ? dexName(detail) : "???");
    gfx->setTextColor(known ? d.accent : UI_INK);
    int glen = strlen(head);
    int gts = (glen <= 13) ? 3 : 2;
    gfx->setTextSize(gts);
    gfx->setCursor(DEX_CX - glen * (gts == 3 ? 9 : 6), gts == 3 ? 56 : 60);
    gfx->print(head);
    if (known) {
      char types[24];
      typeText(types, sizeof(types), d);
      gfx->setTextColor(battleTypeColor(d.type1));
      gfx->setTextSize(2);
      gfx->setCursor(DEX_CX - strlen(types) * 6, 94);
      gfx->print(types);
    }
    if (galleryPmd.loaded) {
      drawPmdActM(galleryPmd, PMD_IDLE, DEX_CX, 300, known ? millis() : 0, true, !known, 6);
    } else {
      const uint8_t *t = thumbs.get(detail);
      if (t) drawThumb(t, DEX_CX - GAL_CELL, 135, 4, !known);
    }
    if (reg) {
      const char *mark = T(S_RAISED_MARK);
      gfx->setTextColor(UI_BAR_OK);
      gfx->setTextSize(2);
      gfx->setCursor(DEX_CX - strlen(mark) * 6, caught ? 354 : 366);
      gfx->print(mark);
    }
    if (caught) {
      const char *mark = T(S_CAUGHT_MARK);
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setTextSize(2);
      gfx->setCursor(DEX_CX - strlen(mark) * 6, reg ? 376 : 366);
      gfx->print(mark);
    }
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    gfx->setCursor(DEX_CX - strlen(T(S_DETAIL_BACK)) * 6, 408);
    gfx->print(T(S_DETAIL_BACK));
    flushStampedFrame();
    return;
  }

  if (!dirty) return;
  dirty = false;

  gfx->fillScreen(UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(DEX_CX - 7 * 9, 28);
  gfx->print("POKEDEX");

  char head[28];
  snprintf(head, sizeof(head), T(S_RAISED_CAUGHT_FMT), pet.registeredCount(), pet.caughtCount());
  gfx->setTextSize(2);
  gfx->setCursor(DEX_CX - strlen(head) * 6, 54);
  gfx->print(head);

  const char *regions[4] = { T(S_FILTER_ALL), "KANTO", "JOHTO", "HOENN" };
  const char *filters[4] = { T(S_FILTER_ALL), T(S_RAISED_MARK), T(S_CAUGHT_MARK), "SHINY" };
  for (int i = 0; i < 4; i++) {
    int fx = 73 + i * 80;
    uint16_t fill = (region == i) ? UI_INK : UI_WHITE;
    uint16_t text = (region == i) ? UI_BG_DAY : UI_INK;
    gfx->fillRoundRect(fx, 68, 76, 16, 5, fill);
    gfx->drawRoundRect(fx, 68, 76, 16, 5, UI_INK);
    gfx->setTextColor(text); gfx->setTextSize(1);
    gfx->setCursor(fx + (76 - (int)strlen(regions[i]) * 6) / 2, 73); gfx->print(regions[i]);
    fill = (filter == i) ? UI_INK : UI_WHITE;
    text = (filter == i) ? UI_BG_DAY : UI_INK;
    gfx->fillRoundRect(fx, 88, 76, 16, 5, fill);
    gfx->drawRoundRect(fx, 88, 76, 16, 5, UI_INK);
    gfx->setTextColor(text); gfx->setTextSize(1);
    gfx->setCursor(fx + (76 - (int)strlen(filters[i]) * 6) / 2, 93); gfx->print(filters[i]);
  }

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      int16_t dex = dexAt(page * 16 + r * 4 + c);
      if (dex <= 0) continue;
      int x = GAL_X + c * GAL_CELL, y = GAL_Y + r * GAL_CELL;
      const uint8_t *t = thumbs.get(dex);
      if (t) {
        bool reg = pet.isRegistered(dex);
        bool caught = pet.isCaught(dex);
        bool known = reg || caught;
        drawThumb(t, x, y, 2, !known);
        if (pet.isShinyRegistered(dex)) {
          gfx->setTextColor(UI_BAR_WARN);
          gfx->setTextSize(2);
          gfx->setCursor(x + 62, y + 4);
          gfx->print("*");
        } else if (caught && !reg) {
          gfx->setTextColor(UI_BAR_WARN);
          gfx->setTextSize(1);
          gfx->setCursor(x + 60, y + 6);
          gfx->print("C");
        }
      } else {
        char num[6];
        snprintf(num, sizeof(num), "%d", dex);
        gfx->setTextColor(UI_TRACK);
        gfx->setTextSize(2);
        gfx->setCursor(x + 24, y + 32);
        gfx->print(num);
      }
    }
  }
  int pages = pageCount();
  int dotX = DEX_CX - (pages - 1) * 7;
  for (int i = 0; i < pages; i++) {
    if (i == page) gfx->fillCircle(dotX + i * 14, 436, 4, UI_INK);
    else gfx->drawCircle(dotX + i * 14, 436, 3, UI_INK);
  }
  flushStampedFrame();
}

void DexScreen::handleTap(int16_t x, int16_t y) {
  if (detail) {
    detail = 0;
    galleryPmd.unload();
    dirty = true;
    sfxPlay(SFX_TAP);
    return;
  }
  if (y < 46) {
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
    return;
  }
  if (y >= 68 && y < GAL_Y) {
    int f = (x - 73) / 80;
    if (f >= 0 && f < 4 && x >= 73 + f * 80 && x <= 149 + f * 80) {
      if (y < 86) region = (uint8_t)f;
      else filter = (uint8_t)f;
      page = 0;
      dirty = true;
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (x < GAL_X || y < GAL_Y) return;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c < 0 || c > 3 || r < 0 || r > 3) return;
  int16_t dex = dexAt(page * 16 + r * 4 + c);
  if (dex <= 0) return;
  showDetail(dex);
  sfxPlay(SFX_MENU);
  if (pet.isRegistered(dex) || pet.isCaught(dex)) speciesChirpPlay(dex);
}

void DexScreen::handleSwipe(int dir) {
  if (detail) {
    detail = 0;
    galleryPmd.unload();
    dirty = true;
    lockTouchBrief();
    return;
  }
  int nextPage = page - dir;
  int maxPage = pageCount() - 1;
  if (nextPage < 0) {
    setScreen(nullptr);
    sfxPlay(SFX_TAP);
    return;
  }
  if (nextPage > maxPage) nextPage = maxPage;
  if (nextPage != page) {
    page = nextPage;
    dirty = true;
    sfxPlay(SFX_MENU);
  }
}
