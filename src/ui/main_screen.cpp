#include "main_screen.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../../audio.h"
#include "../../battle.h"
#include "../../dex.h"
#include "../../dayphase.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../rtcbat.h"
#include "../../sdmon.h"
#include "../../species.h"
#include "../../species_chirp.h"
#include "../../species_chirp.h"
#include "../../time_utils.h"
#include "dex_screen.h"
#include "battle_screen.h"
#include "party_screen.h"
#include "game_ball_screen.h"
#include "game_catch_screen.h"
#include "game_clean_screen.h"
#include "game_memo_screen.h"
#include "game_type_screen.h"
#include "pet_card_page.h"
#include "screen.h"
#include "strength_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon evoPmd;
extern bool gNight;
extern uint32_t eggWiggleUntil;
extern uint32_t morningUntil;
extern uint32_t wildPromptUntil;
extern uint32_t feedMenuUntil;
extern uint32_t confirmUntil;
extern uint32_t choiceUntil;
extern uint8_t choiceKind;
extern bool gameMenuOpen;

extern int sceneHour();
extern uint16_t inkColor();
extern void drawScene(uint8_t biome, uint32_t now, bool night);
extern void drawHeader(const char *name, uint16_t nameColor, const char *msg);
extern void drawCeremony();
extern void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette);
extern void drawPet();
extern void drawBath();
extern void drawPoops();
extern void drawBars();
extern void drawButtons();
extern void drawGameMenu();
extern const char *eggMsg();
extern const char *statusMsg();
extern void flushStampedFrame();
extern const char *dexName(int16_t dex);
static void behNext();
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);
extern void openClock(Screen *returnTo);
extern bool inPetZone(int16_t x, int16_t y);
extern int8_t gameMenuHit(int16_t x, int16_t y);
extern void startGameMenuChoice(uint8_t idx);

static constexpr int CX = 233;
static constexpr int PET_CY = 202;
static constexpr uint16_t INK_K = 0x18C4;
static const uint8_t CRACK1[][2] = { {15,8},{16,9},{15,10} };
static const uint8_t CRACK2[][2] = { {11,13},{12,14},{11,15},{20,12},{19,13},{20,14} };

extern bool screenOff;
extern bool cardDirty;
extern uint8_t cardPage;
extern bool expeditionTrainChoiceOpen;
extern uint32_t bathUntil;
extern uint32_t nextWildEligible;
extern uint32_t lastWildCheck;
extern uint32_t nextPetEventEligible;
extern uint32_t lastPetEventCheck;
extern uint32_t petEventUntil;
extern uint32_t petEventFeedbackUntil;
extern uint8_t petEventType;
extern char petEventMsg[18];
extern int16_t wildPromptDex;
extern uint8_t wildPromptLevel;
extern bool wildPromptShiny;
extern const char *medalName(int index);
extern uint8_t currentDayPhase();
#define WILD_COOLDOWN_MS (20UL * 60UL * 1000UL)
#define WILD_PROMPT_MS 20000UL
#define WILD_CHECK_MS 60000UL
#define PET_EVENT_COOLDOWN_MS (15UL * 60UL * 1000UL)
#define PET_EVENT_PROMPT_MS 18000UL
#define PET_EVENT_CHECK_MS 60000UL
static bool mainScreenReadyForWild();
static void maybeOfferWildEncounter(uint32_t now);
static void scheduleNextPetEvent(uint32_t now);
static bool mainScreenReadyForPetEvent();
static void maybeOfferPetEvent(uint32_t now);
static void maybeOfferMorning(uint32_t now);
static void drawWildPrompt();
static void drawPetEvent();
static void drawStreakBadge();
static void drawCelebration();
static void drawChoiceDialog();
static void drawEvolveButton();
static void drawFarewellButton();
static void drawRunawayButton();
void MainScreen::update(uint32_t now) {
  maybeOfferWildEncounter(now);
  maybeOfferPetEvent(now);
  maybeOfferMorning(now);
}

void MainScreen::scheduleNextWild(uint32_t now) {
  nextWildEligible = now + WILD_COOLDOWN_MS;
}

void MainScreen::beginReleaseConfirmation() {
  confirmUntil = millis() + 10000;
}

bool MainScreen::handleReleaseTap(int16_t x, int16_t y) {
  if (!confirmUntil) return false;
  if (deadlineActive(millis(), confirmUntil) &&
      x >= 118 && x <= 218 && y >= 252 && y <= 304) {
    pet.release();
  }
  confirmUntil = 0;
  return true;
}

bool MainScreen::handleWildPromptTap(int16_t x, int16_t y) {
  if (!wildPromptUntil) return false;
  if (deadlineActive(millis(), wildPromptUntil)) {
    bool fight = (x >= 93 && x <= 373 && y >= 226 && y <= 270);
    bool later = (x >= 93 && x <= 373 && y >= 278 && y <= 322);
    if (fight) {
      battleScreen.start(wildPromptDex, wildPromptLevel, wildPromptShiny ? 1 : 0);
    } else if (later) {
      wildPromptUntil = 0;
      mainScreen.scheduleNextWild(millis());
      sfxPlay(SFX_TAP);
    }
  } else {
    wildPromptUntil = 0;
  }
  return true;
}

bool MainScreen::handleChoiceTap(int16_t x, int16_t y) {
  if (!choiceKind) return false;
  if (choiceKind == 3) {
    uint8_t options = pet.evolutionOptionCount();
    for (uint8_t i = 0; i < options; i++) {
      int optionY = 158 + i * 45;
      if (x >= 82 && x <= 394 && y <= 194 + i * 45 && y >= optionY) {
        int16_t target = pet.evolutionOption(i);
        int16_t old = pet.speciesId;
        pet.evolveTo(target);
        evoPmd.load(old, pet.shiny);
        choiceKind = 0;
        return true;
      }
    }
    if (y >= 392 && y <= 444) choiceKind = 0;
    return true;
  }

  bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);
  bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);
  if (choiceKind == 1) {
    if (b1) {
      if (pet.evolutionOptionCount() > 1) {
        choiceKind = 3;
        choiceUntil = millis() + 12000;
        return true;
      }
      int16_t old = pet.speciesId;
      pet.evolveTo(pet.evolutionOption(0));
      evoPmd.load(old, pet.shiny);
    } else if (b2) {
      pet.declineEvolve();
    }
  } else if (choiceKind == 2) {
    if (b1) pet.startFarewell();
    else if (b2) pet.declineFarewell();
  }
  choiceKind = 0;
  return true;
}

MainScreen mainScreen;

/** Maps an expedition item to its localized label string. */
StrId expeditionItemText(ExpeditionItem item) {
  switch (item) {
    case EXP_ITEM_SNACK: return S_ITEM_SNACK;
    case EXP_ITEM_ENERGY: return S_ITEM_ENERGY;
    case EXP_ITEM_CARE: return S_ITEM_CARE;
    case EXP_ITEM_TRAIN: return S_ITEM_TRAIN;
    default: return S_WAIT;
  }
}

/** Maps an expedition site to its localized label string. */
StrId expeditionSiteText(ExpeditionSite site) {
  if (site == EXP_SITE_VOLCANO) return S_EXP_VOLCANO;
  if (site == EXP_SITE_FOREST) return S_EXP_FOREST;
  return S_EXP_COAST;
}

/** Returns the display color assigned to an expedition site. */
uint16_t expeditionSiteColor(ExpeditionSite site) {
  return site == EXP_SITE_VOLCANO ? UI_BAR_BAD : site == EXP_SITE_FOREST ? UI_BAR_OK : 0x4C98;
}

/** Returns the display color assigned to an expedition item. */
uint16_t expeditionItemColor(ExpeditionItem item) {
  switch (item) {
    case EXP_ITEM_SNACK: return UI_BAR_WARN;
    case EXP_ITEM_ENERGY: return 0x4C98;
    case EXP_ITEM_CARE: return UI_BAR_OK;
    case EXP_ITEM_TRAIN: return UI_BAR_BAD;
    default: return UI_TRACK;
  }
}

/** Returns the current RTC time, falling back to the pet's saved timestamp. */
uint32_t expeditionNowEpoch() {
  uint32_t nowEpoch = rtcEpoch();
  return nowEpoch ? nowEpoch : pet.lastSeenEpoch;
}

static const int EXP_HUD_X = 310;
static const int EXP_HUD_Y = 106;
static const int EXP_HUD_W = 112;
static const int EXP_HUD_H = 34;
static const int STEP_HUD_X = 130;
static const int STEP_HUD_Y = 24;
static const int STEP_HUD_W = 126;
static const int STEP_HUD_H = 22;

/** Tests whether a point lies inside the steps HUD hit area. */
bool inStepsHudHit(int16_t x, int16_t y) {
  return x >= STEP_HUD_X && x <= STEP_HUD_X + STEP_HUD_W &&
         y >= STEP_HUD_Y && y <= STEP_HUD_Y + STEP_HUD_H;
}

/** Opens the steps card and closes any expedition training-choice overlay. */
void openStepsCard() {
  cardPage = 8;
  expeditionTrainChoiceOpen = false;
  cardDirty = true;
  setScreen(petCardScreenForPage(cardPage));
  sfxPlay(SFX_MENU);
}

/** Draws the current daily step count on the home screen. */
void drawStepsHud() {
  uint16_t color = pet.stepsToday >= 5000UL ? UI_BAR_WARN :
                   pet.stepsToday >= 500UL ? UI_BAR_OK : UI_TRACK;
  char label[24];
  snprintf(label, sizeof(label), "%s %lu", T(S_STEPS), (unsigned long)pet.stepsToday);
  gfx->fillRoundRect(STEP_HUD_X, STEP_HUD_Y, STEP_HUD_W, STEP_HUD_H, 8, UI_WHITE);
  gfx->drawRoundRect(STEP_HUD_X, STEP_HUD_Y, STEP_HUD_W, STEP_HUD_H, 8, color);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(STEP_HUD_X + (STEP_HUD_W - (int)strlen(label) * 6) / 2, STEP_HUD_Y + 10);
  gfx->print(label);
}

/** Returns whether the expedition HUD can currently be shown. */
bool expeditionHudVisible() {
  if (pet.isEgg() || pet.ceremony || activeScreen) return false;
  if (gameMenuOpen || feedMenuUntil || confirmUntil || choiceKind || bathUntil ||
      petEventUntil || wildPromptUntil) return false;
  return pet.expeditionHudState(expeditionNowEpoch()) != EXP_HUD_HIDDEN;
}

/** Tests whether a point lies inside the visible expedition HUD hit area. */
bool inExpeditionHudHit(int16_t x, int16_t y) {
  return expeditionHudVisible() && x >= EXP_HUD_X && x <= EXP_HUD_X + EXP_HUD_W &&
         y >= EXP_HUD_Y && y <= EXP_HUD_Y + EXP_HUD_H;
}

/** Opens the expedition card and closes any training-choice overlay. */
void openExpeditionCard() {
  cardPage = 7;
  expeditionTrainChoiceOpen = false;
  cardDirty = true;
  setScreen(petCardScreenForPage(cardPage));
  sfxPlay(SFX_MENU);
}

/** Draws the expedition status HUD when it is available. */
void drawExpeditionHud() {
  if (!expeditionHudVisible()) return;

  uint32_t nowEpoch = expeditionNowEpoch();
  ExpeditionHudState state = pet.expeditionHudState(nowEpoch);
  uint16_t color = state == EXP_HUD_READY ? UI_BAR_OK :
                   state == EXP_HUD_BAG ? UI_BAR_WARN : 0x4C98;
  char label[18];
  if (state == EXP_HUD_ACTIVE) {
    uint32_t left = (pet.expeditionEndEpoch - nowEpoch + 59UL) / 60UL;
    snprintf(label, sizeof(label), "%s %lum", T(expeditionSiteText((ExpeditionSite)pet.expeditionSite)), (unsigned long)left);
  } else if (state == EXP_HUD_READY) {
    snprintf(label, sizeof(label), "%s", T(S_EXP_READY));
  } else {
    snprintf(label, sizeof(label), "%s x%u", T(S_EXP_HUD_BAG), pet.expeditionItemCount());
  }

  gfx->fillRoundRect(EXP_HUD_X, EXP_HUD_Y, EXP_HUD_W, EXP_HUD_H, 8, UI_WHITE);
  gfx->drawRoundRect(EXP_HUD_X, EXP_HUD_Y, EXP_HUD_W, EXP_HUD_H, 8, color);
  gfx->fillCircle(EXP_HUD_X + 17, EXP_HUD_Y + 17, 9, color);
  gfx->setTextColor(UI_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(EXP_HUD_X + 14, EXP_HUD_Y + 12);
  if (state == EXP_HUD_READY) gfx->print("!");
  else if (state == EXP_HUD_BAG) gfx->print("+");
  else gfx->print(">");
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(EXP_HUD_X + 31, EXP_HUD_Y + 13);
  gfx->print(label);
}

void MainScreen::updateWalkCards(uint16_t walked, uint32_t stepsBefore, uint32_t stepDayBefore) {
  if (walked && activeScreen == petCardScreenForPage(7)) cardDirty = true;
  if ((pet.stepsToday != stepsBefore || pet.stepDay != stepDayBefore) &&
      activeScreen == petCardScreenForPage(8)) cardDirty = true;
}

void MainScreen::updateExpedition(uint32_t now) {
  static uint32_t lastExpeditionCardTick = 0;
  if (activeScreen == petCardScreenForPage(7) && now - lastExpeditionCardTick >= 1000UL) {
    lastExpeditionCardTick = now;
    cardDirty = true;
  }

  static bool expeditionWasActive = false;
  bool expeditionActiveNow = pet.expeditionActive(pet.lastSeenEpoch);
  if (expeditionWasActive && !expeditionActiveNow && pet.expeditionEndEpoch) {
    sfxPlay(SFX_EXPEDITION_FOUND);
    if (activeScreen == petCardScreenForPage(7)) cardDirty = true;
  }
  expeditionWasActive = expeditionActiveNow;
}

extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);
extern uint32_t pmdActTotalMs(const PmdAct &a);
extern bool bathPending;
extern bool gameMenuDirty;
extern uint8_t currentDayPhase();
extern void drawBar(int x, int y, const char *label, uint8_t val);
#define HORIZON 232
#define BTN_HALF 26
#define BTN_HIT 36
#define EVO_BTN_W 256
#define EVO_BTN_H 64
#define EVO_BTN_X (CX - EVO_BTN_W / 2)
#define EVO_BTN_Y 172
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
#define BTN_HIT 36
#ifndef C565
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#endif

MainButton buttons[4] = {
  { 140, 390, SPR_ICON_FOOD },
  { 202, 404, SPR_ICON_PLAY },
  { 264, 404, SPR_ICON_LIGHT },
  { 326, 390, SPR_ICON_CLEAN },
};

const GameMenuTile GAME_MENU_TILES[5] = {
  { 88, 156, 138, 58, 0 },
  { 240, 156, 138, 58, 1 },
  { 88, 226, 138, 58, 2 },
  { 240, 226, 138, 58, 3 },
  { 94, 296, 278, 62, 4 },
};

/** Returns the game-menu tile at the point, or -1 if there is no hit. */
int8_t gameMenuHit(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 5; i++) {
    const GameMenuTile &t = GAME_MENU_TILES[i];
    if (x >= t.x && x <= t.x + t.w && y >= t.y && y <= t.y + t.h) return i;
  }
  return -1;
}

/** Starts the minigame selected by its menu-tile index. */
void startGameMenuChoice(uint8_t idx) {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  switch (GAME_MENU_TILES[idx].game) {
    case 0: gameBallScreen.start(); break;
    case 1: gameCatchScreen.start(); break;
    case 2: gameMemoScreen.start(); break;
    case 3: gameCleanScreen.start(); break;
    case 4: gameTypeScreen.start(); break;
  }
}

void drawPet();
void drawPetPMD();
void drawPetSD();


void MainScreen::render() {
  int h = sceneHour();
  gNight = isVisualNight(h, pet.sleeping);
  drawScene(pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome, millis(), gNight);

  if (pet.ceremony) {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    const char *msg = (pet.ceremony == CER_FAREWELL) ? T(S_FAREWELL)
                      : (pet.ceremony == CER_RUNAWAY) ? T(S_RUNAWAY)
                                                      : T(S_GOODBYE);
    drawHeader(dexName(pet.speciesId), d.accent, msg);
    drawCeremony();
    flushStampedFrame();
    return;
  }

  if (pet.isEgg()) {
    drawHeader(T(S_EGG_HDR), inkColor(), eggMsg());
    int s = 5, x = CX - 16 * s, y = PET_CY - 16 * s;
    if (deadlineActive(millis(), eggWiggleUntil))
      x += ((millis() / 40) % 2) ? 6 : -6;
    drawMap(SPR_EGG, SPRITE_H, x, y, s, false);
    if (pet.eggCracks() >= 1)
      for (auto &c : CRACK1) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggCracks() >= 2)
      for (auto &c : CRACK2) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggRarity() >= R_RARO) {
      const char *rar = (pet.eggRarity() == R_LEGENDARIO) ? T(S_EGG_LEGEND) : T(S_EGG_RARE);
      gfx->setTextColor(pet.eggRarity() == R_LEGENDARIO ? UI_BAR_WARN : 0x4C98);
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(rar) * 6, 316);
      gfx->print(rar);
    }
    char reg[24];
    snprintf(reg, sizeof(reg), T(S_POKEDEX_FMT), pet.registeredCount(), DEX_COUNT);
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    gfx->setTextColor(inkColor());
    gfx->setTextSize(2);
    gfx->setCursor(CX - strlen(reg) * 6, 348);
    gfx->print(reg);
    drawStepsHud();
  } else {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    char name[28];
    const char *base = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
    snprintf(name, sizeof(name), T(S_NAME_FMT), pet.shiny ? "*" : "", base, pet.level());
    drawHeader(name, gNight ? UI_INK_NIGHT : d.accent, statusMsg());
    drawStreakBadge();
    drawStepsHud();
    drawPet();
    drawBath();
    drawPoops();
    drawExpeditionHud();
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    drawBars();
    drawButtons();
    drawCelebration();
    drawPetEvent();
    if (gameMenuOpen) drawGameMenu();
    if (pet.wantEvolveButton()) drawEvolveButton();
    else if (pet.canRunawayNow()) drawRunawayButton();
    else if (pet.wantFarewellButton()) drawFarewellButton();
  }

  if (pet.sleeping) {
    gfx->setTextColor(UI_INK_NIGHT);
    gfx->setTextSize(3);
    gfx->setCursor(320, 130);
    gfx->print("Zz");
  }

  if (morningUntil) {
    if (deadlineReached(millis(), morningUntil)) morningUntil = 0;
    else {
      const char *hello = T(S_GOOD_MORNING);
      gfx->fillRoundRect(90, 78, 286, 44, 14, UI_WHITE);
      gfx->drawRoundRect(90, 78, 286, 44, 14, inkColor());
      gfx->setTextColor(inkColor());
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(hello) * 6, 92);
      gfx->print(hello);
    }
  }

  if (wildPromptUntil) {
    if (deadlineReached(millis(), wildPromptUntil)) wildPromptUntil = 0;
    else drawWildPrompt();
  }

  if (feedMenuUntil) {
    if (deadlineReached(millis(), feedMenuUntil)) {
      feedMenuUntil = 0;
    } else {
      gfx->fillRoundRect(101, 288, 264, 64, 14, UI_WHITE);
      gfx->drawRoundRect(101, 288, 264, 64, 14, inkColor());
      drawMap(SPR_ICON_FOOD, 16, 110, 296, 3, false);
      drawMap(SPR_ICON_BERRY_B, 16, 176, 296, 3, false);
      drawMap(SPR_ICON_BERRY_G, 16, 242, 296, 3, false);
      drawMap(SPR_ICON_CANDY, 16, 308, 296, 3, false);
    }
  }

  if (confirmUntil) {
    if (deadlineReached(millis(), confirmUntil)) {
      confirmUntil = 0;
    } else {
      gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
      gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
      char q[28];
      snprintf(q, sizeof(q), T(S_RELEASE_FMT), dexName(pet.speciesId));
      gfx->setTextColor(UI_INK);
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(q) * 6, 196);
      gfx->print(q);
      gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      gfx->setCursor(118 + (100 - (int)strlen(T(S_YES)) * 12) / 2, 270);
      gfx->print(T(S_YES));
      gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
      gfx->setCursor(248 + (100 - (int)strlen(T(S_NO)) * 12) / 2, 270);
      gfx->print(T(S_NO));
    }
  }

  if (choiceKind) {
    if (deadlineReached(millis(), choiceUntil)) choiceKind = 0;
    else drawChoiceDialog();
  }

  flushStampedFrame();
}

extern SdMon mon;
extern PmdMon pmd;
extern PmdMon evoPmd;
extern PetBehavior beh;
extern uint32_t pmdActTotalMs(const PmdAct &a);
extern int flashIdxForDex(int16_t dex);
extern void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS);
extern void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS);
extern void overlayEye(const Species &sp, int x, int y, int s, int col);
extern void overlayMouth(const Species &sp, int x, int y, int s, bool open);
static constexpr int PET_GROUND = 304;
#ifndef C565
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#endif
/** Draws the active farewell or runaway ceremony animation. */
void drawCeremony() {
  if (!pmd.loaded) { drawPet(); return; }  // respaldo si no hay sprite PMD
  uint32_t now = millis();
  float t = pet.ceremonyT();               // Progress from 0 to 1 over ten seconds.
  bool panic = (pet.ceremony == CER_RUNAWAY);
  int x = CX, y = PET_GROUND;
  uint8_t act = PMD_IDLE;

  if (panic) {
    // The sad ending uses blue shadows and rain.
    for (int i = 0; i < 46; i++) {
      int rx = (i * 47 + now / 3) % 466;
      int ry = (i * 91 + now / 2) % 470;
      gfx->drawLine(rx, ry, rx - 3, ry + 12, C565(0x6a, 0x84, 0xb0));
    }
    bool fade = false;
    if (t < 0.30f) {                       // Head lowered and trembling.
      act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
      x = CX + (int)(4 * sinf(now * 0.04f));
    } else {                               // Slowly walks away and fades.
      act = pmd.has(PMD_WALKL) ? PMD_WALKL : PMD_IDLE;
      x = CX - (int)(((t - 0.30f) / 0.70f) * (CX + 120));
      fade = (t > 0.6f) && ((now / 160) % 2 == 0);  // Flicker into a silhouette.
    }
    drawPmdAct(act, x, y, now, true, fade, 5);  // Fade into a silhouette as it leaves.
    // Draw a falling tear.
    if (t < 0.55f) {
      int ty = y - 150 + (int)((now / 6) % 40);
      gfx->fillRect(x + 6, ty, 3, 6, C565(0x9a, 0xc4, 0xe8));
    }
    return;
  }

  // The farewell uses a pulsing golden halo, sparks, and rising hearts.
  int gcy = PET_GROUND - 96;
  for (int k = 0; k < 4; k++) {
    int r = 60 + k * 34 + (int)(10 * sinf(now * 0.02f));
    gfx->drawCircle(CX, gcy, r, C565(0xff, 0xdf, 0x8a));
  }
  for (int i = 0; i < 16; i++) {
    int px = (i * 71 + 28) % 466;
    int py = 410 - (int)((now / 8 + i * 70) % 360);   // Rise and reappear at the bottom.
    if (py < 30) continue;
    if (i % 4 == 0) drawMap(SPR_HEART, 32, px - 8, py - 8, 1, false);  // Small heart.
    else gfx->fillRect(px, py, 4, 4, (i % 2) ? C565(0xff, 0xe7, 0x9f) : C565(0xff, 0x9a, 0xc0));
  }

  if (t < 0.45f) {                         // Bow or farewell pose.
    act = pmd.has(PMD_POSE) ? PMD_POSE : (pmd.has(PMD_NOD) ? PMD_NOD : PMD_IDLE);
  } else {                                 // Walk away to the right.
    act = pmd.has(PMD_WALKR) ? PMD_WALKR : PMD_IDLE;
    x = CX + (int)(((t - 0.45f) / 0.55f) * (CX + 140));
  }
  drawPmdAct(act, x, y, now, true, false, 5);
  if (pet.showHeart())                     // Large heart following the pet.
    drawMap(SPR_HEART, 32, x + 50, y - 190, 2, false);
}

/** Draws the evolving pet's silhouette, rays, sparks, and reveal flash. */
void drawEvolveFX(uint32_t now) {
  float t = pet.evolveT();          // 0..1
  int cx = CX, cy = PET_GROUND - 96;

  // Expanding, pulsing radial halo.
  int halo = 36 + (int)(t * 150) + (int)(8 * sinf(now * 0.02f));
  for (int k = 0; k < 4; k++) {
    int r = halo - k * 7;
    if (r > 0) gfx->drawCircle(cx, cy, r, UI_WHITE);
  }
  // Rotating rays from the pet's center.
  float base = now * 0.004f;
  for (int i = 0; i < 12; i++) {
    float a = base + i * (float)(PI / 6);
    int len = 90 + (int)(70 * (0.5f + 0.5f * sinf(now * 0.012f + i)));
    gfx->drawLine(cx, cy, cx + (int)(cosf(a) * len), cy + (int)(sinf(a) * len), UI_WHITE);
  }
  // Alternate between the old and new silhouettes, accelerating until the new
  // form remains visible for the final reveal flash.
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && evoPmd.loaded && ((now / period) % 2) == 0;
  if (showOld) drawPmdActM(evoPmd, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  // Outward-moving sparks.
  for (int i = 0; i < 10; i++) {
    float a = i * (float)(PI / 5) + t * 4.0f;
    int d = (int)((now / 14 + i * 33) % 200);
    int sx = cx + (int)(cosf(a) * d), sy = cy + (int)(sinf(a) * d);
    gfx->fillRect(sx - 2, sy - 2, 5, 5, (i & 1) ? C565(0xff, 0xe0, 0x70) : UI_WHITE);
  }
  // Final flash before revealing the new form.
  if (t > 0.9f) gfx->fillCircle(cx, cy, (int)(300 * (t - 0.9f) / 0.1f), UI_WHITE);
}

/** Draws the pet using the highest-priority available sprite source. */
void drawPet() {
  if (pmd.loaded) {
    drawPetPMD();
    return;
  }
  if (mon.loaded) {
    drawPetSD();
    return;
  }
  int fi = flashIdxForDex(pet.speciesId);
  if (fi < 0) {
    // No SD or built-in sprite is available; show a clear missing-sprite notice.
    gfx->setTextColor(inkColor());
    gfx->setTextSize(6);
    gfx->setCursor(CX - 18, PET_CY - 80);
    gfx->print("?");
    gfx->setTextSize(2);
    const char *l1 = T(S_NO_SPRITES);
    gfx->setCursor(CX - (int)strlen(l1) * 6, PET_CY - 4);
    gfx->print(l1);
    const char *l2 = T(S_LOAD_SPRITES);
    gfx->setCursor(CX - (int)strlen(l2) * 6, PET_CY + 20);
    gfx->print(l2);
    return;
  }
  const Species &sp = SPECIES[fi];
  int s = sp.scale;
  int x = CX - 16 * s;
  int y = PET_CY - 16 * s;

  // Alternate the previous and current species silhouettes during evolution.
  if (pet.evolving()) {
    bool flash = (millis() / 300) % 2;
    int16_t showDex = (flash && pet.prevSpeciesId >= 0) ? pet.prevSpeciesId : pet.speciesId;
    int sfi = flashIdxForDex(showDex);
    if (sfi >= 0) {
      const Species &show = SPECIES[sfi];
      drawMap(show.sprite, SPRITE_H, CX - 16 * show.scale, PET_CY - 16 * show.scale, show.scale, flash);
    }
    return;
  }

  PetMood m = pet.mood();
  if (m == MOOD_HAPPY && (millis() / 500) % 2) y -= 6;  // Small happy bounce.

  drawMap(sp.sprite, SPRITE_H, x, y, s, false);

  // Overlay expressions using the species-specific facial anchors.
  bool blink = (millis() % 3500 < 300);
  if (m == MOOD_SLEEPING || blink) {
    overlayEye(sp, x, y, s, sp.eyeColL);
    overlayEye(sp, x, y, s, sp.eyeColR);
  }
  if (m == MOOD_EATING) overlayMouth(sp, x, y, s, true);
  else if (m == MOOD_SAD) overlayMouth(sp, x, y, s, false);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + 20 * s, y - 2 * s, 2, false);
}

/** Draws the active pet using its multi-action PMD sprite. */
void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // Release the previous form after evolution.

  PetMood m = pet.mood();
  uint8_t act;
  bool loop = true;
  if (m == MOOD_SLEEPING && pmd.has(PMD_SLEEP)) {
    act = PMD_SLEEP;
    beh.mode = 0;
  } else if (m == MOOD_EATING && pmd.has(PMD_EAT)) {
    act = PMD_EAT;
    beh.t0 = 0;
  } else if (m == MOOD_SAD && pmd.has(PMD_HURT)) {
    act = PMD_HURT;
  } else {
    // For a happy pet, the behavior scheduler chooses idle, walking, or a gesture.
    if (deadlineReached(now, beh.until)) behNext();
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        behNext();
        act = PMD_IDLE;
      } else {
        beh.x += (d > 0 ? 3.0f : -3.0f);
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
      }
    } else {
      act = (beh.mode == 2) ? beh.act : PMD_IDLE;
      loop = false;
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  drawPmdAct(act, (int)beh.x, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, 5);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, (int)beh.x + 50, PET_GROUND - 190, 2, false);
}

/** Draws the active pet using the legacy animated SD sprite format. */
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // Small happy bounce.
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    for (int c = 0; c < mon.w; c++) {
      uint8_t idx = row[c];
      if (idx == 0xFF) continue;
      gfx->fillRect(x + c * s, y + r * s, s, s, sil ? INK_K : mon.pal[idx]);
    }
  }

  // Use emotes because imported sprites do not include facial anchors.
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
}

/** Draws an eye over a built-in sprite using its species-specific anchor. */
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

/** Draws an open or closed mouth over a built-in species sprite. */
void overlayMouth(const Species &sp, int x, int y, int s, bool open) {
  int mc = sp.mouthCol, mr = sp.mouthRow;
  gfx->fillRect(x + (mc - 3) * s, y + mr * s, 7 * s, 2 * s, sp.bodyColor);
  if (open) {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, 2 * s, INK_K);
  } else {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, s, INK_K);
    gfx->fillRect(x + (mc - 3) * s, y + (mr + 1) * s, s, s, INK_K);
    gfx->fillRect(x + (mc + 3) * s, y + (mr + 1) * s, s, s, INK_K);
  }
}

/** Returns whether a random wild encounter can be offered on the home screen. */
static bool mainScreenReadyForWild() {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (activeScreen || gameMenuOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || petEventUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}

/** Rolls and displays a random wild encounter when its cooldown expires. */
static void maybeOfferWildEncounter(uint32_t now) {
  if (nextWildEligible == 0) {
    mainScreen.scheduleNextWild(now);
    return;
  }
  if (wildPromptUntil) return;
  if (now - lastWildCheck < WILD_CHECK_MS) return;
  lastWildCheck = now;
  if (deadlineActive(now, nextWildEligible)) return;
  if (!mainScreenReadyForWild()) return;
  int16_t cand = pickWildSpecies((uint8_t)random(100), currentDayPhase());
  uint8_t phase = currentDayPhase();
  uint8_t chance = (phase == 3) ? 4 : (phase == 0 ? 7 : 8);
  if (phase == 3 && cand >= 1 && cand <= DEX_COUNT) {
    const DexEntry &d = DEX_TBL[cand];
    if (d.type1 == TYPE_GHOST || d.type2 == TYPE_GHOST ||
        d.type1 == TYPE_POISON || d.type2 == TYPE_POISON) chance = 8;
  }
  if ((uint8_t)random(100) >= chance) return;

  wildPromptDex = cand;
  wildPromptLevel = wildLevelFor(pet.level(), (uint8_t)random(100));
  wildPromptShiny = (random(4096) < pet.stepShinyChancePer4096());
  wildPromptUntil = now + WILD_PROMPT_MS;
  mainScreen.scheduleNextWild(now);
  sfxPlay(SFX_MENU);
}

/** Sets the eligibility time for the next random pet event. */
static void scheduleNextPetEvent(uint32_t now) {
  nextPetEventEligible = now + PET_EVENT_COOLDOWN_MS;
}

/** Returns whether a random pet event can be offered on the home screen. */
static bool mainScreenReadyForPetEvent() {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (activeScreen || gameMenuOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}

/** Rolls and displays a random pet event when its cooldown expires. */
static void maybeOfferPetEvent(uint32_t now) {
  if (nextPetEventEligible == 0) {
    scheduleNextPetEvent(now);
    return;
  }
  if (petEventUntil) {
    if (deadlineReached(now, petEventUntil)) petEventUntil = 0;
    return;
  }
  if (now - lastPetEventCheck < PET_EVENT_CHECK_MS) return;
  lastPetEventCheck = now;
  if (deadlineActive(now, nextPetEventEligible)) return;
  if (!mainScreenReadyForPetEvent()) return;
  uint8_t phase = currentDayPhase();
  uint8_t chance = (phase == 0) ? 14 : (phase == 3 ? 8 : 10);
  if ((uint8_t)random(100) >= chance) return;

  petEventType = (uint8_t)random(3);
  petEventUntil = now + PET_EVENT_PROMPT_MS;
  scheduleNextPetEvent(now);
  sfxPlay(SFX_EVENT_SPARKLE);
}

bool MainScreen::inPetEventHit(int16_t x, int16_t y) const {
  int16_t ex = 366, ey = 286;
  int dx = x - ex, dy = y - ey;
  return dx * dx + dy * dy <= 44 * 44;
}

void MainScreen::acceptPetEvent() {
  uint8_t type = petEventType;
  petEventUntil = 0;
  scheduleNextPetEvent(millis());
  if (!pet.applyPetEvent(type)) return;
  StrId msg = S_EVENT_FOUND;
  if (type == PET_EVENT_HEART) msg = S_EVENT_PET;
  else if (type == PET_EVENT_SPARKLE) msg = S_EVENT_LUCKY;
  snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(msg));
  petEventFeedbackUntil = millis() + 1800;
  sfxPlay(type == PET_EVENT_BERRY ? SFX_EAT : (type == PET_EVENT_SPARKLE ? SFX_EVENT_SPARKLE : SFX_HEART));
}

bool MainScreen::readyForShake() const {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (activeScreen || gameMenuOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil) return false;
  if (pet.evolving()) return false;
  return true;
}

void MainScreen::handleSwipe(int dir) {
  if (pet.ceremony || confirmUntil) return;
  if (dir < 0) {
    dexScreen.openGrid();
    setScreen(&dexScreen);
    sfxPlay(SFX_MENU);
  } else if (!pet.isEgg()) {
    setScreen(&partyScreen);
    sfxPlay(SFX_MENU);
  }
}

void MainScreen::handleSwipeV(int dir) {
  if (pet.ceremony) return;
  if (dir > 0) {
    if (!confirmUntil && !feedMenuUntil) openClock(nullptr);
  } else if (!pet.isEgg() && !confirmUntil && !feedMenuUntil) {
    cardPage = 0;
    cardDirty = true;
    setScreen(petCardScreenForPage(cardPage));
    sfxPlay(SFX_MENU);
  }
}

/** Displays the once-per-day morning greeting when the home screen is available. */
static void maybeOfferMorning(uint32_t now) {
  if (morningUntil) {
    if (deadlineReached(now, morningUntil)) morningUntil = 0;
    return;
  }
  if (screenOff || pet.awaitingStarter() || pet.ceremony) return;
  if (activeScreen || gameMenuOpen) return;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil || petEventUntil) return;
  if (currentDayPhase() != 0) return;
  if (!pet.takeMorningGreeting()) return;
  morningUntil = now + 2500UL;
  if (!pet.sleeping && !pet.isEgg() && audioMode() == SOUND_FULL)
    speciesChirpPlay(pet.speciesId, true);
  else if (!pet.sleeping && !pet.isEgg())
    sfxPlay(SFX_HEART);
}

/** Draws the prompt asking whether to start the pending wild battle. */
static void drawWildPrompt() {
  gfx->fillRoundRect(82, 156, 302, 178, 18, UI_WHITE);
  gfx->drawRoundRect(82, 156, 302, 178, 18, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(T(S_WILD_Q)) * 9, 176);
  gfx->print(T(S_WILD_Q));
  char name[28];
  snprintf(name, sizeof(name), "%s%s Lv.%u", wildPromptShiny ? "*" : "",
           dexName(wildPromptDex), wildPromptLevel);
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(name) * 6, 206);
  gfx->print(name);

  gfx->fillRoundRect(93, 226, 280, 44, 12, UI_BAR_BAD);
  gfx->fillRoundRect(93, 278, 280, 44, 12, UI_TRACK);
  gfx->setTextColor(UI_WHITE);
  gfx->setCursor(CX - strlen(T(S_FIGHT)) * 6, 240);
  gfx->print(T(S_FIGHT));
  gfx->setTextColor(UI_BG_DAY);
  gfx->setCursor(CX - strlen(T(S_LATER)) * 6, 292);
  gfx->print(T(S_LATER));
}

/** Draws the pending pet event and any interaction feedback message. */
static void drawPetEvent() {
  uint32_t now = millis();
  if (petEventUntil && deadlineReached(now, petEventUntil)) petEventUntil = 0;
  if (!petEventUntil && (!petEventFeedbackUntil || deadlineReached(now, petEventFeedbackUntil))) return;

  int16_t ex = 366, ey = 286;
  if (petEventUntil) {
    gfx->fillCircle(ex, ey, 32, UI_WHITE);
    gfx->drawCircle(ex, ey, 34, UI_BAR_WARN);
    if (petEventType == PET_EVENT_BERRY) {
      drawMap(SPR_ICON_BERRY_G, 16, ex - 24, ey - 24, 3, false);
    } else if (petEventType == PET_EVENT_HEART) {
      drawMap(SPR_HEART, 32, ex - 32, ey - 32, 2, false);
    } else {
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setTextSize(4);
      gfx->setCursor(ex - 12, ey - 18);
      gfx->print("*");
      gfx->fillCircle(ex - 14, ey + 12, 4, UI_WHITE);
      gfx->fillCircle(ex + 16, ey + 10, 4, UI_WHITE);
    }
  }
  if (petEventFeedbackUntil && deadlineActive(now, petEventFeedbackUntil)) {
    gfx->setTextColor(UI_BAR_WARN);
    gfx->setTextSize(2);
    gfx->setCursor(CX - strlen(petEventMsg) * 6, 292);
    gfx->print(petEventMsg);
  }
}

/** Draws the player's current care-streak badge. */
static void drawStreakBadge() {
  if (pet.streak < 1) return;
  int x = 26, y = 16;
  gfx->fillTriangle(x + 8, y, x + 1, y + 17, x + 15, y + 17, UI_BAR_BAD);
  gfx->fillTriangle(x + 8, y + 7, x + 4, y + 17, x + 12, y + 17, UI_BAR_WARN);
  char s[6];
  snprintf(s, sizeof(s), "%u", pet.streak);
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(x + 22, y + 2);
  gfx->print(s);
}

/** Draws the active medal, Dex, streak, or step-reward celebration. */
static void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  char buf[20];
  if (pet.showMedal()) {
    for (int i = 0; i < MED_COUNT; i++)
      if (pet.newMedal & (1 << i)) { l2 = medalName(i); break; }
    l1 = T(S_MEDAL_BANNER);
  } else if (pet.showDexReward()) {
    l1 = T(S_NEW_FRAME);
    l2 = T((StrId)(S_RANK_TRAINER + pet.collectionRank()));
  } else if (pet.showMilestone()) {
    snprintf(buf, sizeof(buf), T(S_STREAK_DAYS_FMT), pet.streak);
    l1 = T(S_GREAT);
    l2 = buf;
  } else if (pet.showStepReward()) {
    l1 = T(S_STEP_REWARD);
    if (pet.lastStepReward() == STEP_REWARD_TRAIL_RANK) {
      snprintf(buf, sizeof(buf), T(S_TRAIL_RANK_FMT), pet.stepTrailRank());
      l2 = buf;
    } else if (pet.lastStepReward() == STEP_REWARD_ENERGY) {
      l2 = T(S_ITEM_ENERGY);
    } else if (pet.lastStepReward() == STEP_REWARD_TRAIN) {
      l2 = T(S_ITEM_TRAIN);
    } else {
      l2 = T(S_ITEM_SNACK);
    }
  }
  if (!l1) return;
  gfx->fillRoundRect(73, 150, 320, 96, 16, UI_BAR_WARN);
  gfx->drawRoundRect(73, 150, 320, 96, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(l1) * 9, 176);
  gfx->print(l1);
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(l2) * 6, 212);
  gfx->print(l2);
}

/** Draws an evolution, farewell, or target-selection choice dialog. */
static void drawChoiceDialog() {
  if (choiceKind == 3) {
    gfx->fillRoundRect(52, 96, 356, 350, 18, UI_WHITE);
    gfx->drawRoundRect(52, 96, 356, 350, 18, UI_INK);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    const char *title = T(S_EVO_Q);
    gfx->setCursor(CX - (int)strlen(title) * 6, 116);
    gfx->print(title);
    uint8_t options = pet.evolutionOptionCount();
    for (uint8_t i = 0; i < options; i++) {
      int16_t target = pet.evolutionOption(i);
      int y = 158 + i * 45;
      gfx->fillRoundRect(82, y, 312, 36, 10, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      const char *name = dexName(target);
      gfx->setCursor(CX - (int)strlen(name) * 6, y + 11);
      gfx->print(name);
    }
    gfx->fillRoundRect(82, 392, 312, 40, 10, UI_TRACK);
    gfx->setTextColor(UI_INK);
    const char *cancel = T(S_EVO_KEEP);
    gfx->setCursor(CX - (int)strlen(cancel) * 6, 404);
    gfx->print(cancel);
    return;
  }
  const char *q, *o1, *o2;
  uint16_t c1, c2, t1, t2;
  if (choiceKind == 1) {  // Evolution.
    q = T(S_EVO_Q); o1 = T(S_EVO_TAP); o2 = T(S_EVO_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else {                // Farewell.
    q = T(S_FAR_Q); o1 = T(S_FAR_GO); o2 = T(S_FAR_STAY);
    c1 = UI_BAR_WARN; t1 = UI_INK; c2 = UI_BAR_OK; t2 = UI_WHITE;
  }
  gfx->fillRoundRect(73, 156, 320, 188, 16, UI_WHITE);
  gfx->drawRoundRect(73, 156, 320, 188, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(q) * 6, 176);
  gfx->print(q);
  gfx->fillRoundRect(93, 206, 280, 52, 12, c1);     // Action button.
  gfx->setTextColor(t1);
  gfx->setCursor(CX - (int)strlen(o1) * 6, 224);
  gfx->print(o1);
  gfx->fillRoundRect(93, 268, 280, 52, 12, c2);     // Keep or stay button.
  gfx->setTextColor(t2);
  gfx->setCursor(CX - (int)strlen(o2) * 6, 286);
  gfx->print(o2);
}

/** Draws the animated button that starts evolution. */
static void drawEvolveButton() {
  uint32_t now = millis();
  int p = (int)(5 * sinf(now * 0.006f));  // late: -5..5
  int x = EVO_BTN_X - p, y = EVO_BTN_Y - p, w = EVO_BTN_W + 2 * p, h = EVO_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 18, UI_BAR_BAD);
  gfx->drawRoundRect(x, y, w, h, 18, UI_WHITE);
  gfx->drawRoundRect(x + 2, y + 2, w - 4, h - 4, 16, UI_WHITE);
  gfx->setTextColor(UI_WHITE);
  gfx->setTextSize(3);
  const char *t = T(S_EVO_TAP);
  gfx->setCursor(CX - (int)strlen(t) * 9, y + h / 2 - 11);
  gfx->print(t);
}

/** Draws the animated button that starts a farewell ceremony. */
static void drawFarewellButton() {
  uint32_t now = millis();
  int p = (int)(4 * sinf(now * 0.005f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, UI_BAR_WARN);
  gfx->drawRoundRect(x, y, w, h, 16, UI_INK);
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_FAREWELL_BTN), nm);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(buf) * 6, y + h / 2 - 8);
  gfx->print(buf);
}

/** Draws the button for the pet's runaway ceremony. */
static void drawRunawayButton() {
  uint32_t now = millis();
  int p = (int)(3 * sinf(now * 0.003f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, C565(0x3a, 0x44, 0x5a));
  gfx->drawRoundRect(x, y, w, h, 16, C565(0x70, 0x80, 0x98));
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_RUNAWAY_BTN), nm);
  gfx->setTextColor(C565(0xc8, 0xd2, 0xe0));
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(buf) * 6, y + h / 2 - 8);
  gfx->print(buf);
}

/** Draws two clouds moving across the daytime sky. */
static void drawClouds(uint32_t now, uint16_t col) {
  for (int k = 0; k < 2; k++) {
    int cx = (int)((now / 50 + k * 250) % 560) - 40;
    int cy = 70 + k * 34;
    gfx->fillCircle(cx, cy, 16, col);
    gfx->fillCircle(cx + 18, cy + 3, 13, col);
    gfx->fillCircle(cx - 15, cy + 4, 12, col);
  }
}

/** Draws the sky, celestial objects, ground, and biome details. */
void drawScene(uint8_t biome, uint32_t now, bool night) {
  int h = sceneHour();
  uint16_t top, bot;
  if (night)            { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (h < 8)       { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // Sunrise.
  else if (h < 18)      { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // Daytime.
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // Sunset.

  // Draw the sky in color bands.
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // Draw the sun or moon.
  if (night) {
    gfx->fillCircle(360, 78, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 72, 22, lerp565(top, bot, 78, HORIZON));  // Crescent moon.
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (h < 18) {
    gfx->fillCircle(360, 84, 26, h < 8 ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // Setting sun.
  }

  // Draw a band of water over the sand at the beach biome.
  uint16_t soil = BIOME_SOIL[biome < 6 ? biome : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (biome == 1) {
    uint16_t sea = night ? C565(0x1c, 0x34, 0x52) : C565(0x4f, 0x96, 0xc4);
    gfx->fillRect(0, HORIZON - 26, 466, 26, sea);
    for (int i = 0; i < 3; i++) {
      int wy = HORIZON - 22 + i * 7;
      uint16_t fc = night ? C565(0x3a, 0x58, 0x78) : C565(0xbf, 0xe6, 0xf5);
      gfx->fillRect(60 + ((now / 60 + i * 30) % 60), wy, 26, 2, fc);
      gfx->fillRect(300 - ((now / 60 + i * 20) % 60), wy, 26, 2, fc);
    }
  }

  // Draw the ground.
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // Add biome-specific details.
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // Forest: silhouetted conifers.
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // Volcano: rocks and embers.
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // Mountain: distant peaks.
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // Snow: falling flakes.
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // Grassland: tufts of grass.
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

/** Draws the main-screen menu of available minigames. */
void drawGameMenu() {
  gameMenuDirty = false;
  gfx->fillRoundRect(78, 112, 310, 266, 18, UI_WHITE);
  gfx->drawRoundRect(78, 112, 310, 266, 18, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  const char *title = T(S_PLAY);
  gfx->setCursor(CX - strlen(title) * 9, 124);
  gfx->print(title);
  const char *labels[5] = { T(S_GAME_BALL), T(S_GAME_CATCH), T(S_GAME_MEMO), T(S_GAME_CLEAN), T(S_GAME_TYPE) };
  uint16_t cols[5] = { UI_BAR_BAD, UI_BAR_WARN, 0x4C98, UI_BAR_OK, 0xF3B7 };
  for (int i = 0; i < 5; i++) {
    const GameMenuTile &t = GAME_MENU_TILES[i];
    gfx->fillRoundRect(t.x, t.y, t.w, t.h, 14, cols[i]);
    gfx->drawRoundRect(t.x, t.y, t.w, t.h, 14, UI_INK);
    gfx->setTextColor(i == 1 ? UI_INK : UI_BG_DAY);
    gfx->setTextSize(2);
    gfx->setCursor(t.x + (t.w - (int)strlen(labels[i]) * 12) / 2, t.y + (t.h - 16) / 2);
    gfx->print(labels[i]);
  }
}

/** Draws the battery charge or charging indicator. */
void drawBattery() {
  int pc = batPercent();
  if (pc < 0) return;  // No battery is connected.
  int x = CX - 14, y = 12, w = 24, h = 11;
  bool charging = batCharging();
  uint16_t col = charging ? UI_BAR_OK
                 : (pc >= 40) ? inkColor()
                 : (pc >= 15) ? UI_BAR_WARN
                              : UI_BAR_BAD;
  gfx->drawRoundRect(x, y, w, h, 2, col);
  gfx->fillRect(x + w, y + 3, 3, 5, col);  // Battery terminal.
  if (charging) {
    // Draw a charging bolt instead of the charge-level bar.
    uint16_t bolt = C565(0xff, 0xd9, 0x4a);
    int bx = x + w / 2;
    gfx->fillTriangle(bx + 3, y + 1, bx - 4, y + 6, bx + 1, y + 6, bolt);
    gfx->fillTriangle(bx - 1, y + 5, bx + 4, y + 5, bx - 3, y + 10, bolt);
  } else {
    int fw = (w - 4) * pc / 100;
    if (fw > 0) gfx->fillRect(x + 2, y + 2, fw, h - 4, col);
  }
}

/** Draws the pet name, status message, and battery indicator. */
void drawHeader(const char *name, uint16_t nameColor, const char *msg) {
  drawBattery();
  gfx->setTextColor(nameColor);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(name) * 9, 52);
  gfx->print(name);
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(msg) * 6, 90);
  gfx->print(msg);
}

void MainScreen::startBath() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || bathUntil) return;
  bathUntil = millis() + 3000;
  bathPending = true;
  sfxPlay(SFX_EVENT_SPARKLE);
  int cx = (int)beh.x;
  for (auto &b : bubbles) {
    b.x = cx - 70 + random(140);
    b.y = PET_GROUND - random(150);
    b.r = 8 + random(16);
    b.ph = random(64);
  }
}

/** Animates bath bubbles and applies the cleaning action when finished. */
void drawBath() {
  if (!bathUntil) return;
  uint32_t now = millis();
  if (deadlineReached(now, bathUntil)) {
    bathUntil = 0;
    if (bathPending) {
      bathPending = false;
      pet.clean();
      // Use a happy pose after cleaning when available.
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]) * 2;
      }
    }
    return;
  }
  uint32_t left = bathUntil - now;
  if (left > 800) {
    // Foam bubbles sway and rise gradually.
    float t = now / 220.0f;
    for (auto &b : bubbles) {
      int bx = b.x + (int)(sinf(t + b.ph) * 6);
      int by = b.y - (int)((3000 - left) / 90);
      gfx->fillCircle(bx, by, b.r, UI_WHITE);
      gfx->drawCircle(bx, by, b.r, 0x7E3D);
      gfx->fillCircle(bx - b.r / 3, by - b.r / 3, b.r / 4, UI_BG_DAY);
    }
  } else {
    // Pop the bubbles into small flashes.
    for (int i = 0; i < 8; i++) {
      auto &b = bubbles[i];
      int sx = b.x + (i % 3) * 6 - 6, sy = b.y - 18;
      uint16_t col = (i % 2) ? UI_BAR_WARN : UI_WHITE;
      gfx->fillRect(sx - 6, sy - 1, 13, 3, col);
      gfx->fillRect(sx - 1, sy - 6, 3, 13, col);
    }
  }
}

/** Draws the pet's on-screen waste. */
void drawPoops() {
  for (int i = 0; i < pet.poops; i++) {
    drawMap(SPR_POOP, 32, 36 + i * 46, 244, 2, false);
  }
}

/** Draws the four care-stat bars. */
void drawBars() {
  drawBar(78, 318, T(S_BAR_FOOD), pet.fullness);
  drawBar(244, 318, T(S_BAR_JOY), pet.joy);
  drawBar(78, 346, T(S_BAR_ENE), pet.energy);
  drawBar(244, 346, T(S_BAR_HYG), pet.hygiene);
}

/** Draws one labeled care-stat bar. */
void drawBar(int x, int y, const char *label, uint8_t val) {
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(x, y);
  gfx->print(label);
  int bx = x + 48, bw = 100, bh = 15;  // Leave room for four-character labels.
  uint16_t fill = (val >= 50) ? UI_BAR_OK : (val >= 25) ? UI_BAR_WARN : UI_BAR_BAD;
  gfx->fillRoundRect(bx, y, bw, bh, 4, UI_TRACK);
  int fw = (bw - 4) * val / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, bh - 4, 3, fill);
}

/** Draws the four home-screen action buttons. */
void drawButtons() {
  for (int i = 0; i < 4; i++) {
    bool off = pet.sleeping && i != 2;  // Only the light button works while asleep.
    int bx = buttons[i].cx - BTN_HALF, by = buttons[i].cy - BTN_HALF;
    if (!pet.sleeping) gfx->fillRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, UI_WHITE);
    gfx->drawRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, inkColor());
    if (!off) drawMap(buttons[i].icon, 16, buttons[i].cx - 16, buttons[i].cy - 16, 2, false);
  }
}

/** Chooses the pet's next idle, walking, or expressive behavior. */
static void behNext() {
  uint32_t now = millis();
  beh.t0 = now;
  int r = random(100);
  uint8_t walkChance = (currentDayPhase() == 3) ? 10 : 35;
  if (r < walkChance && (pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR))) {
    beh.mode = 1;  // Walk.
    beh.targetX = 150 + random(176);
    beh.until = now + 15000;
  } else if (r < 60) {
    // Pick an available gesture. Hop jumps too high; Sit faces away.
    static const uint8_t flair[] = { PMD_POSE, PMD_NOD, PMD_BREATH };
    uint8_t pick[3], n = 0;
    for (uint8_t f : flair)
      if (pmd.has(f)) pick[n++] = f;
    if (n) {
      beh.mode = 2;
      beh.act = pick[random(n)];
      beh.until = now + pmdActTotalMs(pmd.acts[beh.act]);
      return;
    }
    beh.mode = 0;
    beh.until = now + 2000 + random(3000);
  } else {
    beh.mode = 0;  // Face forward.
    beh.until = now + 2000 + random(3000);
  }
}

/** Returns the localized prompt for the egg's current tap count. */
const char *eggMsg() {
  switch (pet.eggCracks()) {
    case 0: return T(S_EGG_TOUCH);
    case 1: return T(S_EGG_MOVES);
    default: return T(S_EGG_ALMOST);
  }
}

/** Returns the localized status message appropriate for the pet's state. */
const char *statusMsg() {
  if (pet.evolving()) return T(S_EVOLVING);
  if (bathUntil) return "Splish splash!";  // Universal onomatopoeia.
  if (pet.sleeping) return "Zzz...";
  if (pet.eating()) return T(S_EATING);
  if (pet.showHeart()) return T(S_LIKES);
  if (pet.fullness < 25) return T(S_HUNGRY);
  if (pet.hygiene < 25) return T(S_NEEDS_BATH);
  if (pet.energy < 25) return T(S_EXHAUSTED);
  if (pet.joy < 25) return T(S_SAD);
  if (pet.weight > 60) return T(S_CHUBBY);
  if (pet.shiny && pet.ageMinutes < 15) return T(S_IS_SHINY);
  return T(S_HAPPY);
}


void MainScreen::handleTap(int16_t x, int16_t y) {
  if (pet.ceremony) return;  // Ignore button taps during the farewell.
  if (petEventUntil && deadlineActive(millis(), petEventUntil) && mainScreen.inPetEventHit(x, y)) {
    mainScreen.acceptPetEvent();
    return;
  }
  if (gameMenuOpen) {
    int8_t hit = gameMenuHit(x, y);
    if (hit >= 0) { lockTouchBrief(140); startGameMenuChoice((uint8_t)hit); }
    else { gameMenuOpen = false; markUiDirty(); lockTouchBrief(); sfxPlay(SFX_TAP); }
    return;
  }
  if (mainScreen.handleWildPromptTap(x, y)) return;
  if (inExpeditionHudHit(x, y)) {
    openExpeditionCard();
    return;
  }
  if (inStepsHudHit(x, y)) {
    openStepsCard();
    return;
  }
  if (mainScreen.handleChoiceTap(x, y)) return;
  if (confirmUntil) {        // "Release?" confirmation: yes or no.
    mainScreen.handleReleaseTap(x, y);
    return;
  }
  if (feedMenuUntil) {       // Food selector.
    if (deadlineActive(millis(), feedMenuUntil) && y >= 288 && y <= 352 && x >= 101 && x <= 365) {
      int item = (x - 101) / 66;
      if (item == 3) pet.feedCandy();
      else pet.feedBerry(item);
      sfxPlay(SFX_EAT);
    }
    feedMenuUntil = 0;
    return;
  }
  if (pet.isEgg()) {
    pet.eggTap();
    sfxPlay(SFX_TAP);
    return;
  }
  // The evolution button opens the evolve-or-keep dialog.
  if (pet.wantEvolveButton() && x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    if (pet.canEvolveNow()) {
      choiceKind = 1; choiceUntil = millis() + 12000;
    } else {
      StrId blocked = pet.evolutionNeedsMoreBond() ? S_EVO_NEED_BOND : S_EVO_BLOCKED;
      snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(blocked));
      petEventFeedbackUntil = millis() + 1800;
      sfxPlay(SFX_DENY);
    }
    return;
  }
  // The shared ending button starts a runaway or opens the farewell dialog.
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { pet.startRunaway(); return; }
    if (pet.wantFarewellButton()) { choiceKind = 2; choiceUntil = millis() + 12000; return; }
  }
  for (int i = 0; i < 4; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT) {
      Serial.printf("BTN %d\n", i);
      sfxPlay(SFX_TAP);
      if (i == 0) {
        if (!pet.sleeping) {
          feedMenuUntil = millis() + 6000;
          sfxPlay(SFX_MENU);
        }
      } else if (i == 1) {
        if (!pet.sleeping) {
          gameMenuOpen = true;
          gameMenuDirty = true;
          lockTouchBrief();
          sfxPlay(SFX_MENU);
        }
      } else if (i == 2) {
        pet.toggleLight();
      } else {
        mainScreen.startBath();
      }
      return;
    }
  }
  // Tapping the pet gives it affection.
  if (inPetZone(x, y)) {
    Serial.println("PET");
    uint8_t r = pet.interactPet(currentDayPhase() == 2);
    // The species chirp is immediate feedback for every pet tap and must not
    // depend on the ten-minute care reward.
    if (!pet.sleeping && audioMode() == SOUND_FULL) speciesChirpPlay(pet.speciesId, true);
    if (r == PET_INTERACT_NONE) return;
    StrId msg = S_WAIT;
    if (r & PET_INTERACT_BOND) msg = S_BOND_GAIN;
    else if (r & PET_INTERACT_JOY) msg = S_HAPPY_FB;
    snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(msg));
    petEventFeedbackUntil = millis() + 1600;
    if (!pet.sleeping && audioMode() != SOUND_FULL)
      sfxPlay((r & PET_INTERACT_BOND) ? SFX_HEART : SFX_TAP);
  }
}
