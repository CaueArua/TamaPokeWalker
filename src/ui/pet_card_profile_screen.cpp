#include "pet_card_profile_screen.h"
#include "pet_card_common.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../audio.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon pmd;
extern bool cardDirty;
extern const char *dexName(int16_t dex);
extern void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t,
                       bool loop, bool sil, uint8_t maxS);
extern void speciesChirpPlay(int16_t dex, bool forced);
extern void openKeyboard();
extern void lockTouchBrief(uint16_t ms = 160);

PetCardProfileScreen petCardProfileScreen;

static uint16_t collectionFrameColor(uint8_t frame) {
  static const uint16_t COLORS[] = {
    UI_TRACK, UI_BAR_WARN, UI_BAR_OK, 0x4C98, UI_BAR_BAD, 0xF3B7, 0x7B5C,
    0x7BEF, 0xD452, 0xFD20, 0x051D,
  };
  return COLORS[frame < sizeof(COLORS) / sizeof(COLORS[0]) ? frame : 0];
}

static void drawFrameCorner(int x, int y, int sx, int sy, uint16_t color, uint8_t len) {
  gfx->drawLine(x, y, x + sx * len, y, color);
  gfx->drawLine(x, y, x, y + sy * len, color);
}

static void drawCollectionFrame(uint8_t frame) {
  if (frame >= pet.unlockedCollectionFrameCount()) frame = 0;
  const int left = 108, right = 358, top = 94, bottom = 302;
  uint16_t color = collectionFrameColor(frame);
  uint8_t len = 22 + frame * 3;
  drawFrameCorner(left, top, 1, 1, color, len);
  drawFrameCorner(right, top, -1, 1, color, len);
  drawFrameCorner(left, bottom, 1, -1, color, len);
  drawFrameCorner(right, bottom, -1, -1, color, len);

  if (frame >= 1) {
    uint16_t accent = frame == 4 ? UI_BAR_WARN : color;
    gfx->fillCircle(left + len + 7, top, 3, accent);
    gfx->fillCircle(right - len - 7, top, 3, accent);
    gfx->fillCircle(left + len + 7, bottom, 3, accent);
    gfx->fillCircle(right - len - 7, bottom, 3, accent);
  }
  if (frame >= 2) {
    uint8_t inner = len - 6;
    drawFrameCorner(left + 6, top + 6, 1, 1, color, inner);
    drawFrameCorner(right - 6, top + 6, -1, 1, color, inner);
    drawFrameCorner(left + 6, bottom - 6, 1, -1, color, inner);
    drawFrameCorner(right - 6, bottom - 6, -1, -1, color, inner);
  }
  if (frame >= 3) {
    gfx->fillRect(PET_CARD_CX - 3, top - 3, 6, 6, color);
    gfx->fillRect(PET_CARD_CX - 3, bottom - 3, 6, 6, color);
  }
  if (frame >= 4) {
    gfx->drawLine(left + 2, top + len + 10, left + 10, top + len + 2, UI_BAR_WARN);
    gfx->drawLine(right - 2, top + len + 10, right - 10, top + len + 2, UI_BAR_WARN);
    gfx->drawLine(left + 2, bottom - len - 10, left + 10, bottom - len - 2, UI_BAR_WARN);
    gfx->drawLine(right - 2, bottom - len - 10, right - 10, bottom - len - 2, UI_BAR_WARN);
  }
  if (frame >= 5) {
    gfx->fillCircle(PET_CARD_CX, top - 12, 3, UI_BAR_WARN);
    gfx->fillCircle(PET_CARD_CX, bottom + 12, 3, UI_BAR_WARN);
    gfx->fillCircle(left - 12, PET_CARD_CX, 3, UI_BAR_WARN);
    gfx->fillCircle(right + 12, PET_CARD_CX, 3, UI_BAR_WARN);
  }
}

static bool inProfilePetPortrait(int16_t x, int16_t y) {
  int dx = x - PET_CARD_CX, dy = y - 206;
  return dx * dx + dy * dy <= 76 * 76;
}

void PetCardProfileScreen::renderPage() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[26];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", nm, pet.level());
  gfx->setTextColor(d.accent);
  int hlen = strlen(head);
  int hts = (hlen <= 11) ? 3 : 2;
  gfx->setTextSize(hts);
  gfx->setCursor(PET_CARD_CX - hlen * (hts == 3 ? 9 : 6), hts == 3 ? 34 : 40);
  gfx->print(head);
  if (pet.nick[0]) {
    gfx->setTextColor(UI_TRACK);
    gfx->setTextSize(2);
    const char *speciesName = dexName(pet.speciesId);
    gfx->setCursor(PET_CARD_CX - (strlen(speciesName) + 2) * 6, 64);
    gfx->printf("(%s)", speciesName);
  }

  drawCollectionFrame(pet.collectionFrame);
  if (pmd.loaded) drawPmdAct(PMD_IDLE, PET_CARD_CX, 206, millis(), true, false, 4);

  int sx = 138, sy = 224;
  gfx->fillTriangle(sx + 8, sy, sx + 1, sy + 18, sx + 15, sy + 18, UI_BAR_BAD);
  gfx->fillTriangle(sx + 8, sy + 7, sx + 4, sy + 18, sx + 12, sy + 18, UI_BAR_WARN);
  char rl[30];
  snprintf(rl, sizeof(rl), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(sx + 24, sy + 2);
  gfx->print(rl);

  drawPetCardStat(258, T(S_VIN), pet.bond, 100, C565(0xd4, 0x52, 0x7e));

  const char *berry = !pet.berryKnown ? T(S_BERRY_UNK)
                      : pet.lovesBerry(0) ? T(S_BERRY_RED)
                      : pet.lovesBerry(1) ? T(S_BERRY_BLUE)
                                          : T(S_BERRY_GREEN);
  char info[40];
  snprintf(info, sizeof(info), T(S_INFO_FMT), berry,
           (unsigned long)(pet.ageMinutes / 1440));
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(PET_CARD_CX - strlen(info) * 6, 296);
  gfx->print(info);

  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(PET_CARD_CX - strlen(T(S_RENAME_HINT)) * 6, 326);
  gfx->print(T(S_RENAME_HINT));

  const char *rank = T((StrId)(S_RANK_TRAINER + pet.collectionRank()));
  char known[22], frame[22];
  snprintf(known, sizeof(known), T(S_KNOWN_FMT), pet.knownDexCount(), DEX_COUNT);
  snprintf(frame, sizeof(frame), T(S_FRAME_FMT), pet.collectionFrame + 1, pet.unlockedCollectionFrameCount());
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(PET_CARD_CX - (int)strlen(T(S_COLLECTION)) * 3, 344);
  gfx->print(T(S_COLLECTION));
  gfx->setTextColor(collectionFrameColor(pet.collectionFrame));
  gfx->setCursor(PET_CARD_CX - (int)strlen(rank) * 3, 356);
  gfx->print(rank);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(PET_CARD_CX - (int)strlen(known) * 3, 368);
  gfx->print(known);

  gfx->fillRoundRect(74, 374, 52, 18, 6, UI_WHITE);
  gfx->drawRoundRect(74, 374, 52, 18, 6, UI_INK);
  gfx->fillRoundRect(340, 374, 52, 18, 6, UI_WHITE);
  gfx->drawRoundRect(340, 374, 52, 18, 6, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(94, 376);
  gfx->print("<");
  gfx->setCursor(360, 376);
  gfx->print(">");
  gfx->setTextSize(1);
  gfx->setCursor(PET_CARD_CX - (int)strlen(frame) * 3, 380);
  gfx->print(frame);
  gfx->setTextSize(2);
}

void PetCardProfileScreen::handlePageTap(int16_t x, int16_t y) {
  if (inProfilePetPortrait(x, y)) {
    speciesChirpPlay(pet.speciesId, true);
    return;
  }
  if (y < 84) {
    openKeyboard();
    return;
  }
  if (y >= 366 && y <= 398) {
    uint8_t count = pet.unlockedCollectionFrameCount();
    uint8_t next = pet.collectionFrame;
    if (x >= 62 && x <= 146) next = pet.collectionFrame == 0 ? count - 1 : pet.collectionFrame - 1;
    else if (x >= 320 && x <= 404) next = (uint8_t)((pet.collectionFrame + 1) % count);
    else return;
    if (pet.setCollectionFrame(next)) {
      cardDirty = true;
      sfxPlay(SFX_MENU);
      lockTouchBrief();
    }
  }
}
