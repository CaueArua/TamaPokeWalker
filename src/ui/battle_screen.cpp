#include "battle_screen.h"
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../battle.h"
#include "../../dayphase.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../species.h"
#include "../../species_chirp.h"
#include "dex_screen.h"
#include "main_screen.h"
#include "party_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern PmdMon pmd;
extern uint32_t wildPromptUntil;
extern uint8_t currentDayPhase();
extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);

extern const char *dexName(int16_t dex);
extern int sceneHour();
extern void drawGameScene();
extern void flushStampedFrame();
extern int drawTypeChips(int x, int y, const DexEntry &d, bool alignRight);
extern void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);
extern void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t,
                       bool loop, bool sil, uint8_t maxS);
extern void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY,
                        uint32_t t, bool loop, bool sil, uint8_t maxS);
extern void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette);
extern uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);

BattleScreen battleScreen;
static constexpr int BATTLE_CX = 233;

bool BattleScreen::isStaticClean() const {
  return battleResolved && !battleDirty;
}

/** Draws a bounded proportional HP bar. */
static void drawBattleHpBar(int x, int y, uint16_t cur, uint16_t maxHp, uint16_t color) {
  if (maxHp == 0) maxHp = 1;
  int w = 150;
  int fw = (int)((uint32_t)cur * w / maxHp);
  if (fw > w) fw = w;
  gfx->fillRoundRect(x, y, w, 14, 4, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(x, y, fw, 14, 4, color);
}

void BattleScreen::battleRewardText(char *buf, size_t len) const {
  if (battleReward.amount == 0) { buf[0] = 0; return; }
  StrId fmt = S_SPD_GAIN_FMT;
  if (battleReward.stat == BATTLE_REWARD_ATK) fmt = S_ATK_GAIN_FMT;
  else if (battleReward.stat == BATTLE_REWARD_DEF) fmt = S_DEF_GAIN_FMT;
  snprintf(buf, len, T(fmt), battleReward.amount);
}

/** Centers a battle button label and reduces its text size when needed. */
static void drawBattleButtonLabel(int x, int y, int w, const char *label) {
  int px = (int)strlen(label) * 12;
  if (px <= w - 8) {
    gfx->setTextSize(2);
    gfx->setCursor(x + (w - px) / 2, y);
    gfx->print(label);
  } else {
    gfx->setTextSize(1);
    gfx->setCursor(x + (w - (int)strlen(label) * 6) / 2, y + 3);
    gfx->print(label);
    gfx->setTextSize(2);
  }
}

void BattleScreen::handleTap(int16_t x, int16_t y) {
  if (battleResolved) {
    if (battleCatchOffered && !battleCatchDone) {
      if (x >= 76 && x <= 224 && y >= 392 && y <= 448) {
        bool closeWin = battleRun.playerHp <= battleRun.playerMaxHp / 3;
        battleCatchTried = true;
        battleCatchDone = true;
        if (battleRespectCatch) {
          battleCatchSuccess = pet.tryRespectCatchWild(battleDex, battleLevel, battlePlayer.level,
                                                       (uint8_t)random(100), battleShiny);
          battleCatchChance = pet.respectCatchChanceForWild(battleDex, battleLevel, battlePlayer.level);
        } else {
          battleCatchSuccess = pet.tryCatchWild(battleDex, battleLevel, battlePlayer.level, closeWin,
                                                (uint8_t)random(100), battleShiny);
          battleCatchChance = pet.catchChanceForWild(battleDex, battleLevel, battlePlayer.level, closeWin);
        }
        if (battleCatchSuccess) {
          bool partyWasFull = pet.isPartyFull();
          pet.registerCaught(battleDex, battleShiny, battleLevel);
          if (partyWasFull) {
            partyReplaceScreen.setCandidate(battleDex, battleLevel, battleShiny);
            setScreen(&partyReplaceScreen);
          }
        }

        sfxPlay(battleCatchSuccess ? SFX_CATCH_OK : SFX_CATCH_FAIL);
        dexScreen.markDirty();
        battleDirty = true;
        return;
      }
      if (x >= 242 && x <= 390 && y >= 392 && y <= 448) {
        battleCatchDone = true;
        battleCatchTried = false;
        battleDirty = true;
        sfxPlay(SFX_TAP);
        return;
      }
      return;
    }
    if (x >= 118 && x <= 348 && y >= 392 && y <= 454) closeBattle();
    return;
  }
  if (battleAttackMenuUntil) {
    if (x >= 66 && x <= 232 && y >= 292 && y <= 352) {
      performBattleAction(BATTLE_ATTACK_QUICK);
      return;
    }
    if (x >= 234 && x <= 400 && y >= 292 && y <= 352) {
      performBattleAction(BATTLE_ATTACK_HEAVY);
      return;
    }
    battleAttackMenuUntil = 0;
    battleDirty = true;
    sfxPlay(SFX_TAP);
    return;
  }
  if (x >= 184 && x <= 282 && y >= 100 && y <= 136) {
    closeBattle();
    sfxPlay(SFX_TAP);
    return;
  }
  if (x >= 46 && x <= 174 && y >= 344 && y <= 428) {
    battleAttackMenuUntil = 1;
    sfxPlay(SFX_TAP);
  } else if (x >= 169 && x <= 297 && y >= 344 && y <= 428) {
    performBattleAction(BATTLE_DODGE);
  } else if (x >= 292 && x <= 420 && y >= 344 && y <= 428) {
    performBattleAction(BATTLE_REST);
  }
}

/** Draws the marker shown for an already-caught battle species. */
static void drawCaughtBattleMarker(int x, int y) {
  drawMap(SPR_ICON_PLAY, 16, x, y, 2, false);
}

void BattleScreen::render() {
  if (battleResolved) battleDirty = false;
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;
  const DexEntry &mine = DEX_TBL[pet.speciesId];
  const DexEntry &wild = DEX_TBL[battleDex];

  gfx->setTextColor(ink);
  gfx->setTextSize(3);
  gfx->setCursor(BATTLE_CX - strlen(T(S_WILD_BATTLE)) * 9, 34);
  gfx->print(T(S_WILD_BATTLE));

  char left[24], right[24];
  snprintf(left, sizeof(left), "%s Lv.%u", pet.nick[0] ? pet.nick : dexName(pet.speciesId), battlePlayer.level);
  snprintf(right, sizeof(right), "%s%s Lv.%u", battleShiny ? "*" : "",
           dexName(battleDex), battleLevel);
  gfx->setTextSize(2);
  gfx->setCursor(28, 82);
  gfx->print(left);
  int rightLen = strlen(right);
  int rightTextSize = rightLen <= 12 ? 2 : 1;
  int rightTextW = rightLen * (rightTextSize == 2 ? 12 : 6);
  int rightX = 408 - rightTextW;
  if (rightX < 246) rightX = 246;
  gfx->setTextSize(rightTextSize);
  gfx->setCursor(rightX, rightTextSize == 2 ? 82 : 88);
  gfx->print(right);
  gfx->setTextSize(2);

  uint16_t playerMax = battleRun.playerMaxHp;
  uint16_t enemyMax = battleRun.enemyMaxHp;
  uint16_t playerCur = battleRun.playerHp;
  uint16_t enemyCur = battleRun.enemyHp;
  drawBattleHpBar(28, 110, playerCur, playerMax, UI_BAR_OK);
  drawBattleHpBar(288, 110, enemyCur, enemyMax, UI_BAR_BAD);
  drawTypeChips(28, 130, mine, false);
  int wildChipLeft = drawTypeChips(438, 130, wild, true);

  if (!battleResolved) {
    gfx->fillRoundRect(188, 102, 90, 32, 9, UI_TRACK);
    gfx->setTextColor(UI_BG_DAY);
    gfx->setTextSize(2);
    gfx->setCursor(188 + (90 - (int)strlen(T(S_RUN_BATTLE)) * 12) / 2, 111);
    gfx->print(T(S_RUN_BATTLE));
  }
  if (pet.isCaught(battleDex))
    drawCaughtBattleMarker(wildChipLeft - 36, 122);

  if (pmd.loaded) drawPmdAct(PMD_IDLE, 142, 286, millis(), true, false, 3);
  else {
    const uint8_t *th = thumbs.get(pet.speciesId);
    if (th) drawThumb(th, 94, 166, 3, false);
  }
  if (wildPmd.loaded) drawPmdActM(wildPmd, PMD_IDLE, 328, 286, millis(), true, false, 3);
  else {
    const uint8_t *th = thumbs.get(battleDex);
    if (th) drawThumb(th, 280, 166, 3, false);
  }

  if (battleResolved) {
    const char *res = battleTurn.playerWon ? T(S_WIN) : T(S_LOSS);
    gfx->setTextColor(battleTurn.playerWon ? UI_BAR_OK : UI_BAR_BAD);
    gfx->setTextSize(4);
    gfx->setCursor(BATTLE_CX - strlen(res) * 12, 300);
    gfx->print(res);
    char rounds[20], damage[28];
    snprintf(rounds, sizeof(rounds), T(S_ROUNDS_FMT), battleRun.round);
    snprintf(damage, sizeof(damage), T(S_DAMAGE_FMT), battleRun.playerDamageTotal, battleRun.enemyDamageTotal);
    gfx->setTextColor(ink);
    gfx->setTextSize(2);
    gfx->setCursor(BATTLE_CX - strlen(rounds) * 6, 334);
    gfx->print(rounds);
    gfx->setCursor(BATTLE_CX - strlen(damage) * 6, 356);
    gfx->print(damage);
    if (battleTurn.playerWon) {
      char reward[20];
      battleRewardText(reward, sizeof(reward));
      if (reward[0]) {
        gfx->setTextColor(UI_BAR_WARN);
        gfx->setCursor(BATTLE_CX - strlen(reward) * 6, 378);
        gfx->print(reward);
      }
    } else if (battleRespectCatch && battleCatchOffered && !battleCatchDone) {
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setTextSize(2);
      gfx->setCursor(BATTLE_CX - strlen(T(S_CLOSE_CHANCE)) * 6, 378);
      gfx->print(T(S_CLOSE_CHANCE));
    }
    if (battleCatchOffered && !battleCatchDone) {
      gfx->fillRoundRect(76, 396, 148, 52, 14, UI_BAR_OK);
      gfx->fillRoundRect(242, 396, 148, 52, 14, UI_TRACK);
      gfx->setTextColor(UI_BG_DAY);
      drawBattleButtonLabel(76, 414, 148, T(S_CATCH_WILD));
      drawBattleButtonLabel(242, 414, 148, T(S_LEAVE_WILD));
    } else {
      if (battleCatchDone && battleCatchTried) {
        const char *catchMsg = battleCatchSuccess ? T(S_CAUGHT_OK) : T(S_ESCAPED);
        gfx->setTextColor(battleCatchSuccess ? UI_BAR_OK : UI_BAR_BAD);
        gfx->setTextSize(2);
        gfx->setCursor(BATTLE_CX - strlen(catchMsg) * 6, 378);
        gfx->print(catchMsg);
      }
      gfx->fillRoundRect(118, 396, 230, 52, 14, UI_BAR_OK);
      gfx->setTextColor(UI_BG_DAY);
      gfx->setTextSize(3);
      gfx->setCursor(BATTLE_CX - strlen(T(S_OK)) * 9, 413);
      gfx->print(T(S_OK));
    }
  } else {
    char roundBuf[14];
    snprintf(roundBuf, sizeof(roundBuf), T(S_ROUND_SHORT_FMT), battleRun.round + 1);
    gfx->setTextColor(ink);
    gfx->setTextSize(2);
    gfx->setCursor(32, 318);
    gfx->print(roundBuf);
    if (battleMsg[0]) {
      gfx->setCursor(BATTLE_CX - strlen(battleMsg) * 6, 318);
      gfx->print(battleMsg);
      if (battleTurn.enemyDamage > 0) {
        char eb[16];
        snprintf(eb, sizeof(eb), "-%u", battleTurn.enemyDamage);
        gfx->setTextColor(UI_BAR_BAD);
        gfx->setCursor(BATTLE_CX - strlen(eb) * 6, 340);
        gfx->print(eb);
      }
    }

    if (battleAttackMenuUntil) {
      gfx->fillRoundRect(74, 298, 150, 46, 12, UI_BAR_BAD);
      gfx->fillRoundRect(242, 298, 150, 46, 12, UI_BAR_WARN);
      gfx->setTextColor(UI_BG_DAY);
      drawBattleButtonLabel(74, 312, 150, T(S_QUICK_ATTACK));
      drawBattleButtonLabel(242, 312, 150, T(S_HEAVY_ATTACK));
    }

    gfx->fillRoundRect(58, 358, 108, 58, 13, UI_BAR_BAD);
    gfx->fillRoundRect(179, 358, 108, 58, 13, 0x4C98);
    gfx->fillRoundRect(300, 358, 108, 58, 13, UI_BAR_OK);
    gfx->setTextColor(UI_BG_DAY);
    drawBattleButtonLabel(58, 380, 108, T(S_ATTACK));
    drawBattleButtonLabel(179, 380, 108, T(S_DODGE));
    char restLabel[18];
    snprintf(restLabel, sizeof(restLabel), "%s %u", T(S_REST), battleRun.restUsesLeft);
    drawBattleButtonLabel(300, 380, 108, restLabel);
  }

  flushStampedFrame();
}

BattleStats BattleScreen::petBattleStats() const {
  BattleStats stats = {};
  stats.level = pet.level();
  stats.atk = pet.atkStat();
  stats.def = pet.defStat();
  stats.spe = pet.speStat();
  stats.hp = 0;
  if (!pet.isEgg() && pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    stats.type1 = DEX_TBL[pet.speciesId].type1;
    stats.type2 = DEX_TBL[pet.speciesId].type2;
  }
  return stats;
}



void BattleScreen::closeBattle() {
  battleResolved = false;
  battleAttackMenuUntil = 0;
  battleCatchOffered = false;
  battleCatchTried = false;
  battleCatchDone = false;
  battleCatchSuccess = false;
  battleRespectCatch = false;
  battleCatchChance = 0;
  wildPmd.unload();
  if (activeScreen == &battleScreen) {
    Screen *target = returnScreen;
    returnScreen = nullptr;
    setScreen(target);
  }
  else { markUiDirty(); lockTouchBrief(); }
}



void BattleScreen::start(int16_t forcedDex, uint8_t forcedLevel, int8_t forcedShiny) {
  if (!canStartWildBattle(pet.isEgg(), pet.sleeping, pet.ceremony)) return;
  returnScreen = activeScreen;
  wildPromptUntil = 0;
  mainScreen.scheduleNextWild(millis());
  if (forcedDex >= 1 && forcedDex <= DEX_COUNT) {
    battleDex = forcedDex;
    battleLevel = forcedLevel ? forcedLevel : wildLevelFor(pet.level(), (uint8_t)random(100));
  } else {
    uint8_t speciesRoll = random(100);
    uint8_t levelRoll = random(100);
    battleDex = pickWildSpecies(speciesRoll, currentDayPhase());
    battleLevel = wildLevelFor(pet.level(), levelRoll);
  }
  battleShiny = forcedShiny < 0 ? (random(4096) < pet.stepShinyChancePer4096()) : forcedShiny != 0;
  battlePlayer = petBattleStats();
  battleEnemy = wildBattleStats(battleDex, battleLevel);
  battleEnemy.hp = 0;
  battleResult = {};
  battleRun = beginBattleRuntime(battlePlayer, battleEnemy);
  battleTurn = {};
  battleReward = {};
  battleMsg[0] = 0;
  battleAttackMenuUntil = 0;
  battleLowHpWarned = false;
  battleCatchOffered = false;
  battleCatchTried = false;
  battleCatchDone = false;
  battleCatchSuccess = false;
  battleRespectCatch = false;
  battleCatchChance = 0;
  battleResolved = false;
  battleDirty = true;
  wildPmd.unload();
  wildPmd.load(battleDex, battleShiny);
  setScreen(&battleScreen);
  sfxPlay(SFX_TAP);
  speciesChirpPlay(battleDex);
}



void BattleScreen::finishBattle() {
  if (battleResolved) return;
  battleResolved = true;
  battleDirty = true;
  if (battleTurn.playerWon) {
    bool closeWin = battleRun.playerHp <= battleRun.playerMaxHp / 3;
    battleReward = pet.applyBattleWin(battleDex, closeWin);
    battleCatchOffered = true;
    battleRespectCatch = false;
    battleCatchChance = pet.catchChanceForWild(battleDex, battleLevel, battlePlayer.level, closeWin);
    sfxPlay(SFX_BATTLE_WIN);
  } else {
    battleReward = {};
    pet.applyBattleLoss();
    bool closeLoss = battleRun.enemyHp > 0 && battleRun.enemyHp * 100UL <= battleRun.enemyMaxHp * 30UL;
    battleCatchChance = closeLoss ? pet.respectCatchChanceForWild(battleDex, battleLevel, battlePlayer.level) : 0;
    battleCatchOffered = battleCatchChance > 0;
    battleRespectCatch = battleCatchOffered;
    sfxPlay(SFX_BATTLE_LOSS);
  }
}



void BattleScreen::performBattleAction(BattleAction action) {
  if (battleResolved) return;
  battleAttackMenuUntil = 0;
  battleLastAction = action;
  battleTurn = stepBattle(battleRun, action, (uint8_t)random(100));
  if (battleTurn.restFailed) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_NO_REST));
  } else if (battleTurn.counterReady) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_COUNTER_READY));
  } else if (battleTurn.playerRested) {
    char healMsg[18];
    snprintf(healMsg, sizeof(healMsg), T(S_RESTED_FMT), battleTurn.playerHeal);
    snprintf(battleMsg, sizeof(battleMsg), "%s %s", healMsg, T(S_GUARD));
  } else if (battleTurn.playerDamage > 0) {
    if (battleTurn.playerTypePct > 100) snprintf(battleMsg, sizeof(battleMsg), "%s %u", T(S_EFFECTIVE), battleTurn.playerDamage);
    else if (battleTurn.playerTypePct < 100) snprintf(battleMsg, sizeof(battleMsg), "%s %u", T(S_NOT_EFFECTIVE), battleTurn.playerDamage);
    else snprintf(battleMsg, sizeof(battleMsg), T(S_HIT_FMT), battleTurn.playerDamage);
  } else if (battleTurn.enemyDodged) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_ENEMY_DODGED));
  } else if (battleTurn.playerDodged) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_DODGED));
  } else {
    snprintf(battleMsg, sizeof(battleMsg), T(S_MISSED));
  }
  if (battleTurn.battleEnded) {
    finishBattle();
    return;
  }
  if (!battleLowHpWarned && battleRun.playerHp > 0 && battleRun.playerHp <= battleRun.playerMaxHp * 3 / 10) {
    battleLowHpWarned = true;
    sfxPlay(SFX_LOW_HP);
  }
  if (battleTurn.restFailed) sfxPlay(SFX_DENY);
  else if (battleTurn.counterReady) sfxPlay(SFX_COUNTER);
  else if (battleTurn.playerRested) sfxPlay(SFX_REST);
  else if (action == BATTLE_ATTACK_QUICK) sfxPlay(SFX_ATTACK_QUICK);
  else if (action == BATTLE_ATTACK_HEAVY) sfxPlay(SFX_ATTACK_HEAVY);
  else if (battleTurn.playerDamage > 0 && battleTurn.playerTypePct > 100) sfxPlay(SFX_EFFECTIVE);
  else if (battleTurn.playerDamage > 0 && battleTurn.playerTypePct < 100) sfxPlay(SFX_WEAK_HIT);
  else if (battleTurn.enemyDamage > 0) sfxPlay(SFX_ENEMY_HIT);
  else sfxPlay(battleTurn.playerDamage > 0 ? SFX_PLAY : SFX_TAP);
}

/** Returns the localized name for a battle type. */
const char *battleTypeName(uint8_t type) {
  static const char *const TYPE_NAMES[LANG_COUNT][19] = {
    { "", "NORMAL", "FUEGO", "AGUA", "ELEC", "PLANTA", "HIELO", "LUCHA", "VENENO", "TIERRA", "VUELO", "PSI", "BICHO", "ROCA", "FANT", "DRAGON", "SINIE", "ACERO", "HADA" },
    { "", "NORMAL", "FIRE", "WATER", "ELEC", "GRASS", "ICE", "FIGHT", "POISON", "GROUND", "FLY", "PSY", "BUG", "ROCK", "GHOST", "DRAGON", "DARK", "STEEL", "FAIRY" },
    { "", "NORMAL", "FEU", "EAU", "ELEC", "PLANTE", "GLACE", "COMBAT", "POISON", "SOL", "VOL", "PSY", "INSECT", "ROCHE", "SPECTRE", "DRAGON", "TENEBR", "ACIER", "FEE" },
    { "", "NORMAL", "FEUER", "WASSER", "ELEKTRO", "PFLANZE", "EIS", "KAMPF", "GIFT", "BODEN", "FLUG", "PSYCHO", "KAEFER", "GESTEIN", "GEIST", "DRACHE", "UNLICHT", "STAHL", "FEE" },
    { "", "NORMALE", "FUOCO", "ACQUA", "ELETTRO", "ERBA", "GHIACCIO", "LOTTA", "VELENO", "TERRA", "VOLANTE", "PSICO", "COLEOT", "ROCCIA", "SPETTRO", "DRAGO", "BUIO", "ACCIAIO", "FOLLETTO" },
    { "", "NORMAL", "FOGO", "AGUA", "ELETR", "PLANTA", "GELO", "LUTA", "VENENO", "TERRA", "VOO", "PSI", "INSETO", "PEDRA", "FANT", "DRAGAO", "SOMBRIO", "ACO", "FADA" },
  };
  if (type < 19) return TYPE_NAMES[gLang][type];
  switch (type) {
    case TYPE_NORMAL: return "NORMAL";
    case TYPE_FIRE: return "FIRE";
    case TYPE_WATER: return "WATER";
    case TYPE_ELECTRIC: return "ELEC";
    case TYPE_GRASS: return "GRASS";
    case TYPE_ICE: return "ICE";
    case TYPE_FIGHTING: return "FIGHT";
    case TYPE_POISON: return "POISON";
    case TYPE_GROUND: return "GROUND";
    case TYPE_FLYING: return "FLY";
    case TYPE_PSYCHIC: return "PSY";
    case TYPE_BUG: return "BUG";
    case TYPE_ROCK: return "ROCK";
    case TYPE_GHOST: return "GHOST";
    case TYPE_DRAGON: return "DRAGON";
    case TYPE_DARK: return "DARK";
    case TYPE_STEEL: return "STEEL";
    case TYPE_FAIRY: return "FAIRY";
  }
  return "";
}



/** Returns the UI color assigned to a battle type. */
uint16_t battleTypeColor(uint8_t type) {
  switch (type) {
    case TYPE_FIRE: return 0xEA87;
    case TYPE_WATER: return 0x4C98;
    case TYPE_ELECTRIC: return 0xBCA1;
    case TYPE_GRASS: return 0x3C49;
    case TYPE_ICE: return 0x5D99;
    case TYPE_FIGHTING: return 0xA2A5;
    case TYPE_POISON: return 0x8A73;
    case TYPE_GROUND: return 0xB447;
    case TYPE_FLYING: return 0x8D7F;
    case TYPE_PSYCHIC: return 0xD28F;
    case TYPE_BUG: return 0x7CC4;
    case TYPE_ROCK: return 0x9407;
    case TYPE_GHOST: return 0x6B33;
    case TYPE_DRAGON: return 0x5A5F;
    case TYPE_DARK: return 0x5ACB;
    case TYPE_STEEL: return 0xA534;
    case TYPE_FAIRY: return 0xF3B7;
    default: return 0x8C4D;
  }
}



/** Formats one or two species types into a bounded text buffer. */
void typeText(char *buf, size_t len, const DexEntry &d) {
  if (d.type2 == TYPE_NONE) snprintf(buf, len, "%s", battleTypeName(d.type1));
  else snprintf(buf, len, "%s %s", battleTypeName(d.type1), battleTypeName(d.type2));
}



/** Calculates the pixel width needed to draw a type chip. */
int typeChipWidth(uint8_t type) {
  return (int)strlen(battleTypeName(type)) * 6 + 14;
}



/** Draws a single colored type chip. */
void drawTypeChip(int x, int y, uint8_t type) {
  if (type == TYPE_NONE) return;
  const char *label = battleTypeName(type);
  int w = typeChipWidth(type);
  gfx->fillRoundRect(x, y, w, 16, 5, lerp565(battleTypeColor(type), UI_WHITE, 5, 8));
  gfx->drawRoundRect(x, y, w, 16, 5, UI_INK);
  gfx->setTextSize(1);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(x + 7, y + 5);
  gfx->print(label);
}



/** Draws a species' type chips and returns their leftmost coordinate. */
int drawTypeChips(int x, int y, const DexEntry &d, bool alignRight) {
  int w1 = typeChipWidth(d.type1);
  int w2 = d.type2 == TYPE_NONE ? 0 : typeChipWidth(d.type2);
  int total = w1 + (w2 ? 4 + w2 : 0);
  int sx = alignRight ? x - total : x;
  drawTypeChip(sx, y, d.type1);
  if (d.type2 != TYPE_NONE) drawTypeChip(sx + w1 + 4, y, d.type2);
  gfx->setTextSize(2);
  return sx;
}






