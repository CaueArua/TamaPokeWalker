#include "pet_card_box_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../audio.h"
#include "dex_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern bool cardOpen;
extern bool cardDirty;
extern const char *dexName(int16_t dex);
extern void typeText(char *buf, size_t cap, const DexEntry &d);
extern uint16_t battleTypeColor(uint8_t t);
extern void setScreen(Screen *next);
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);
extern DexScreen dexScreen;

PetCardBoxScreen petCardBoxScreen;

bool PetCardBoxScreen::comesBefore(int16_t a, int16_t b) const {
  if (sort == 1) {
    const DexEntry &da = DEX_TBL[a];
    const DexEntry &db = DEX_TBL[b];
    if (da.type1 != db.type1) return da.type1 < db.type1;
    if (da.type2 != db.type2) return da.type2 < db.type2;
  } else if (sort == 2) {
    bool ra = pet.isRegistered(a);
    bool rb = pet.isRegistered(b);
    if (ra != rb) return ra;
  }
  return a < b;
}

uint16_t PetCardBoxScreen::buildList(int16_t *out) const {
  uint16_t n = 0;
  for (int16_t dex = 1; dex <= DEX_COUNT; dex++)
    if (pet.isCaught(dex)) out[n++] = dex;
  for (uint16_t i = 1; i < n; i++) {
    int16_t v = out[i];
    int j = i - 1;
    while (j >= 0 && comesBefore(v, out[j])) {
      out[j + 1] = out[j];
      j--;
    }
    out[j + 1] = v;
  }
  return n;
}

uint8_t PetCardBoxScreen::pageCount() const {
  uint16_t count = pet.caughtCount();
  uint8_t pages = (count + ROWS - 1) / ROWS;
  return pages > 0 ? pages : 1;
}

int16_t PetCardBoxScreen::dexAt(uint16_t index) const {
  int16_t list[DEX_COUNT];
  uint16_t n = buildList(list);
  return index < n ? list[index] : 0;
}

const char *PetCardBoxScreen::sortLabel() const {
  if (sort == 1) return T(S_SORT_TYPE);
  if (sort == 2) return T(S_SORT_RAISED);
  return T(S_SORT_DEX);
}

void PetCardBoxScreen::render() {
  uint8_t pages = pageCount();
  if (page >= pages) page = pages - 1;

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_BOX)) * 9, 42);
  gfx->print(T(S_BOX));

  const char *sortText = sortLabel();
  gfx->fillRoundRect(302, 62, 106, 28, 9, UI_WHITE);
  gfx->drawRoundRect(302, 62, 106, 28, 9, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(302 + (106 - (int)strlen(sortText) * 6) / 2, 73);
  gfx->print(sortText);

  char caught[24], known[24], goal[22];
  snprintf(caught, sizeof(caught), T(S_CAUGHT_COUNT_FMT), pet.caughtCount(), DEX_COUNT);
  snprintf(known, sizeof(known), T(S_KNOWN_FMT), pet.knownDexCount(), DEX_COUNT);
  snprintf(goal, sizeof(goal), T(S_DEX_GOAL_FMT), pet.nextDexGoal());
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(72, 70);
  gfx->print(caught);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(72, 92);
  gfx->print(known);
  gfx->setCursor(258, 92);
  gfx->print(goal);

  if (pet.caughtCount() == 0) {
    gfx->fillRoundRect(82, 178, 302, 72, 16, UI_WHITE);
    gfx->drawRoundRect(82, 178, 302, 72, 16, UI_TRACK);
    gfx->setTextColor(UI_TRACK);
    gfx->setTextSize(2);
    gfx->setCursor(PET_CARD_CX - strlen(T(S_NO_CATCHES)) * 6, 207);
    gfx->print(T(S_NO_CATCHES));
    return;
  }

  int16_t boxList[DEX_COUNT];
  uint16_t boxCount = buildList(boxList);
  uint16_t first = (uint16_t)page * ROWS;
  for (uint8_t i = 0; i < ROWS; i++) {
    uint16_t index = first + i;
    int16_t dex = index < boxCount ? boxList[index] : 0;
    if (dex <= 0) break;
    const DexEntry &d = DEX_TBL[dex];
    int y = 122 + i * 42;
    bool raised = pet.isRegistered(dex);
    gfx->fillRoundRect(58, y, 350, 34, 9, UI_WHITE);
    gfx->drawRoundRect(58, y, 350, 34, 9, d.accent);
    char name[24];
    snprintf(name, sizeof(name), "#%03d %s", DEX_TBL[dex].natNum, dexName(dex));
    int ts = strlen(name) <= 16 ? 2 : 1;
    gfx->setTextSize(ts);
    gfx->setTextColor(UI_INK);
    gfx->setCursor(72, y + (ts == 2 ? 7 : 5));
    gfx->print(name);
    char types[22];
    typeText(types, sizeof(types), d);
    gfx->setTextSize(1);
    gfx->setTextColor(battleTypeColor(d.type1));
    gfx->setCursor(72, y + 24);
    gfx->print(types);
    if (raised) {
      gfx->setTextColor(UI_BAR_OK);
      gfx->setCursor(330, y + 15);
      gfx->print(T(S_RAISED_MARK));
    }
  }
  gfx->fillRoundRect(76, 348, 94, 38, 11, page > 0 ? UI_TRACK : C565(0xe4, 0xe8, 0xee));
  gfx->fillRoundRect(296, 348, 94, 38, 11, page + 1 < pages ? UI_TRACK : C565(0xe4, 0xe8, 0xee));
  gfx->setTextColor(UI_BG_DAY);
  gfx->setTextSize(3);
  gfx->setCursor(111, 357);
  gfx->print("<");
  gfx->setCursor(331, 357);
  gfx->print(">");
  char pg[12];
  snprintf(pg, sizeof(pg), T(S_PAGE_FMT), page + 1, pages);
  gfx->setTextColor(UI_TRACK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(pg) * 6, 360);
  gfx->print(pg);
}

void PetCardBoxScreen::handlePageTap(int16_t x, int16_t y) {
  if (x >= 302 && x <= 408 && y >= 62 && y <= 92) {
    sort = (sort + 1) % 3;
    page = 0;
    cardDirty = true;
    sfxPlay(SFX_TAP);
  } else if (x >= 76 && x <= 170 && y >= 348 && y <= 386) {
    if (page > 0) { page--; cardDirty = true; }
    sfxPlay(SFX_TAP);
  } else if (x >= 296 && x <= 390 && y >= 348 && y <= 386) {
    uint8_t pages = pageCount();
    if (page + 1 < pages) { page++; cardDirty = true; }
    sfxPlay(SFX_TAP);
  } else {
    int row = (y - 122) / 42;
    if (row >= 0 && row < 5 && x >= 58 && x <= 408 && y >= 122 + row * 42 && y <= 156 + row * 42) {
      int16_t dex = dexAt((uint16_t)page * 5 + row);
      if (dex > 0) {
        cardOpen = false;
        dexScreen.openDetail(dex);
        setScreen(&dexScreen);
        sfxPlay(SFX_TAP);
        speciesChirpPlay(dex);
      }
    } else if (y >= 400) {
      cardOpen = false;
      markUiDirty();
      lockTouchBrief();
    }
  }
}
