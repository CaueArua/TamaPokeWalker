#include "party_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../pet.h"
#include "../../dex.h"
#include "../../audio.h"

extern Arduino_Canvas *gfx;
extern Pet pet;

extern const char *dexName(int16_t dex);
extern void typeText(char *buf, size_t cap, const DexEntry &d);
extern uint16_t battleTypeColor(uint8_t t);
extern void flushStampedFrame();
extern void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY,
                        uint32_t t, bool loop, bool sil, uint8_t maxS);

static constexpr uint16_t P_EMPTY = 0xE73D;
static constexpr uint16_t P_RED   = 0xD249;
static constexpr int P_CX = 233;

// 1. Coordenadas da grade AMPLIADA da Party (2 linhas x 3 colunas = 400x278px)
static constexpr int LGRID_X0    = 33;
static constexpr int LGRID_Y0    = 84;
static constexpr int LGRID_W     = 128;
static constexpr int LGRID_H     = 134;
static constexpr int LGRID_GAP_X = 8;
static constexpr int LGRID_GAP_Y = 10;

// 2. Coordenadas da grade COMPACTA da tela de Substituição (2 linhas x 3 colunas)
static constexpr int SGRID_X0    = 54;
static constexpr int SGRID_Y0    = 84;
static constexpr int SGRID_W     = 114;
static constexpr int SGRID_H     = 106;
static constexpr int SGRID_GAP_X = 8;
static constexpr int SGRID_GAP_Y = 8;

static void largeCellRect(uint8_t idx, int &x, int &y) {
  x = LGRID_X0 + (idx % 3) * (LGRID_W + LGRID_GAP_X);
  y = LGRID_Y0 + (idx / 3) * (LGRID_H + LGRID_GAP_Y);
}

static void smallCellRect(uint8_t idx, int &x, int &y) {
  x = SGRID_X0 + (idx % 3) * (SGRID_W + SGRID_GAP_X);
  y = SGRID_Y0 + (idx / 3) * (SGRID_H + SGRID_GAP_Y);
}

static uint8_t partySlotLevel(const PartySlot &slot) {
  uint8_t level = 1 + (slot.ageMinutes / MINUTES_PER_LEVEL);
  return level > 100 ? 100 : level;
}

static void loadPartySprites(PmdMon sprites[PARTY_SIZE]) {
  pet.syncToParty();
  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    sprites[i].unload();
    const PartySlot &s = pet.party[i];
    if (s.speciesId > 0 && s.speciesId <= DEX_COUNT) {
      sprites[i].load(s.speciesId, s.shiny);
    }
  }
}

static void unloadPartySprites(PmdMon sprites[PARTY_SIZE]) {
  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    sprites[i].unload();
  }
}

static void drawPartyGrid(PmdMon sprites[PARTY_SIZE], bool largeMode) {
  uint32_t now = millis();
  int cellW = largeMode ? LGRID_W : SGRID_W;
  int cellH = largeMode ? LGRID_H : SGRID_H;
  uint8_t spriteScale = largeMode ? 3 : 2;
  int spriteGroundOffset = largeMode ? 100 : 76;

  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    int x, y;
    if (largeMode) largeCellRect(i, x, y);
    else smallCellRect(i, x, y);

    const PartySlot &s = pet.party[i];
    bool isActive = largeMode && (i == pet.activePartySlot && !pet.isEgg());

    if (s.speciesId > 0 && s.speciesId <= DEX_COUNT) {
      const DexEntry &d = DEX_TBL[s.speciesId];
      uint16_t borderCol = isActive ? UI_BAR_OK : (s.shiny ? UI_BAR_WARN : d.accent);

      gfx->fillRoundRect(x, y, cellW, cellH, 14, UI_WHITE);
      gfx->drawRoundRect(x, y, cellW, cellH, 14, borderCol);
      if (isActive || s.shiny) {
        gfx->drawRoundRect(x + 1, y + 1, cellW - 2, cellH - 2, 13, borderCol);
      }

      if (sprites[i].loaded) {
        drawPmdActM(sprites[i], PMD_IDLE, x + cellW / 2, y + spriteGroundOffset,
                    now, true, false, spriteScale);
      }

      char lvBuf[12];
      snprintf(lvBuf, sizeof(lvBuf), "Lv.%u", partySlotLevel(s));
      gfx->setTextSize(largeMode ? 2 : 1);
      gfx->setTextColor(UI_INK);
      gfx->setCursor(x + 8, y + 8);
      gfx->print(lvBuf);

      if (s.shiny) {
        gfx->setTextSize(2);
        gfx->setTextColor(UI_BAR_WARN);
        gfx->setCursor(x + cellW - 18, y + 6);
        gfx->print("*");
      }

      const char *dispName = (s.nick[0] != '\0') ? s.nick : dexName(s.speciesId);
      int nw = (int)strlen(dispName) * 6;
      gfx->setTextSize(1);
      gfx->setTextColor(isActive ? UI_BAR_OK : UI_INK);
      gfx->setCursor(x + (cellW - nw) / 2, y + cellH - 14);
      gfx->print(dispName);
    } else {
      gfx->fillRoundRect(x, y, cellW, cellH, 14, P_EMPTY);
      gfx->drawRoundRect(x, y, cellW, cellH, 14, UI_TRACK);
      gfx->setTextSize(1);
      gfx->setTextColor(UI_TRACK);
      gfx->setCursor(x + (cellW - 54) / 2, y + cellH / 2 - 4);
      gfx->print("- VAZIO -");
    }
  }
}

PartyScreen partyScreen;
PartyReplaceScreen partyReplaceScreen;

// =========================================================================
// 1. TELA DA PARTY EM GRADE AMPLIADA (2 LINHAS x 3 COLUNAS, SPRITES 3x)
// =========================================================================

void PartyScreen::onEnter() {
  loadPartySprites(slotPmd);
}

void PartyScreen::onExit() {
  unloadPartySprites(slotPmd);
}

void PartyScreen::render() {
  gfx->fillScreen(UI_BG_DAY);

  uint8_t count = 0;
  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    if (pet.party[i].speciesId > 0) count++;
  }

  char title[20];
  snprintf(title, sizeof(title), "PARTY (%u/6)", count);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(P_CX - (int)strlen(title) * 9, 36);
  gfx->print(title);

  drawPartyGrid(slotPmd, true);

  gfx->setTextSize(1);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(P_CX - 57, 386);
  gfx->print("< DESLIZE P/ VOLTAR");

  flushStampedFrame();
}

void PartyScreen::handleTap(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    int cx, cy;
    largeCellRect(i, cx, cy);
    if (x >= cx && x <= cx + LGRID_W && y >= cy && y <= cy + LGRID_H) {
      if (pet.switchPartySlot(i)) {
        sfxPlay(SFX_EAT);
        speciesChirpPlay(pet.speciesId, true);
        setScreen(nullptr);
      }
      return;
    }
  }

  if (y >= 375) {
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
  }
}

void PartyScreen::handleSwipe(int dir) {
  sfxPlay(SFX_TAP);
  setScreen(nullptr);
}

// =========================================================================
// 2. TELA DE SUBSTITUIÇÃO NA CAPTURA (GRADE COMPACTA + BOX DO CAPTURADO)
// =========================================================================

void PartyReplaceScreen::onEnter() {
  loadPartySprites(slotPmd);
  candidatePmd.unload();
  if (pendingDex > 0 && pendingDex <= DEX_COUNT) {
    candidatePmd.load(pendingDex, pendingShiny);
  }
}

void PartyReplaceScreen::onExit() {
  unloadPartySprites(slotPmd);
  candidatePmd.unload();
}

void PartyReplaceScreen::render() {
  gfx->fillScreen(UI_BG_DAY);

  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(P_CX - 90, 42);
  gfx->print("TROCAR QUAL SLOT?");

  drawPartyGrid(slotPmd, false);

  int boxX = 76;
  int boxY = 316;
  int boxW = 314;
  int boxH = 82;
  uint16_t candBorder = pendingShiny ? UI_BAR_WARN : UI_BAR_OK;

  gfx->fillRoundRect(boxX, boxY, boxW, boxH, 14, UI_WHITE);
  gfx->drawRoundRect(boxX, boxY, boxW, boxH, 14, candBorder);
  gfx->drawRoundRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 13, candBorder);

  if (candidatePmd.loaded) {
    drawPmdActM(candidatePmd, PMD_IDLE, boxX + 44, boxY + 66, millis(), true, false, 2);
  }

  gfx->setTextSize(1);
  gfx->setTextColor(pendingShiny ? UI_BAR_WARN : UI_BAR_OK);
  gfx->setCursor(boxX + 86, boxY + 12);
  gfx->print(pendingShiny ? "NOVO *SHINY*" : "CAPTURADO:");

  char candName[28];
  snprintf(candName, sizeof(candName), "#%03d %s", DEX_TBL[pendingDex].natNum, dexName(pendingDex));
  gfx->setTextSize(strlen(candName) <= 13 ? 2 : 1);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(boxX + 86, boxY + 28);
  gfx->print(candName);

  char candLv[12];
  snprintf(candLv, sizeof(candLv), "Lv.%u", pendingLevel);
  gfx->setTextSize(2);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(boxX + 86, boxY + 52);
  gfx->print(candLv);

  gfx->fillRoundRect(boxX + boxW - 86, boxY + 20, 74, 42, 10, P_RED);
  gfx->setTextColor(UI_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(boxX + boxW - 67, boxY + 37);
  gfx->print("SOLTAR");

  flushStampedFrame();
}

void PartyReplaceScreen::handleTap(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < PARTY_SIZE; i++) {
    int cx, cy;
    smallCellRect(i, cx, cy);
    if (x >= cx && x <= cx + SGRID_W && y >= cy && y <= cy + SGRID_H) {
      pet.replacePartySlot(i, pendingDex, pendingShiny, pendingLevel);
      sfxPlay(SFX_MEDAL);
      setScreen(nullptr);
      return;
    }
  }

  int btnX = 76 + 314 - 86;
  int btnY = 316 + 20;
  if (x >= btnX && x <= btnX + 74 && y >= btnY && y <= btnY + 42) {
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
  }
}
