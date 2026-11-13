#include "battle_screen.h"
#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>
#include "../../audio.h"
#include "../../battle.h"
#include "../../dex.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../sdmon.h"
#include "../../species.h"
#include "dex_screen.h"
#include "party_screen.h"

extern Arduino_Canvas *gfx;
extern Pet pet;
extern BattleStats battlePlayer;
extern BattleRuntime battleRun;
extern BattleTurnResult battleTurn;
extern BattleReward battleReward;
extern bool battleResolved;
extern bool battleDirty;
extern int16_t battleDex;
extern uint8_t battleLevel;
extern bool battleShiny;
extern bool battleCatchOffered;
extern bool battleCatchTried;
extern bool battleCatchDone;
extern bool battleCatchSuccess;
extern bool battleRespectCatch;
extern uint8_t battleCatchChance;
extern uint32_t battleAttackMenuUntil;
extern char battleMsg[28];
extern PmdMon pmd;
extern PmdMon wildPmd;

extern const char *dexName(int16_t dex);
extern int sceneHour();
extern void drawGameScene();
extern void flushStampedFrame();
extern void closeBattle();
extern void performBattleAction(BattleAction action);
extern int drawTypeChips(int x, int y, const DexEntry &d, bool alignRight);
extern void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);
extern void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t,
                       bool loop, bool sil, uint8_t maxS);
extern void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY,
                        uint32_t t, bool loop, bool sil, uint8_t maxS);
extern void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette);

BattleScreen battleScreen;
static constexpr int BATTLE_CX = 233;

bool BattleScreen::isStaticClean() const {
  return battleResolved && !battleDirty;
}

static void drawBattleHpBar(int x, int y, uint16_t cur, uint16_t maxHp, uint16_t color) {
  if (maxHp == 0) maxHp = 1;
  int w = 150;
  int fw = (int)((uint32_t)cur * w / maxHp);
  if (fw > w) fw = w;
  gfx->fillRoundRect(x, y, w, 14, 4, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(x, y, fw, 14, 4, color);
}

static void battleRewardText(char *buf, size_t len) {
  if (battleReward.amount == 0) { buf[0] = 0; return; }
  StrId fmt = S_SPD_GAIN_FMT;
  if (battleReward.stat == BATTLE_REWARD_ATK) fmt = S_ATK_GAIN_FMT;
  else if (battleReward.stat == BATTLE_REWARD_DEF) fmt = S_DEF_GAIN_FMT;
  snprintf(buf, len, T(fmt), battleReward.amount);
}

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
          if (!pet.isPartyFull()) {
            pet.registerCaught(battleDex, battleShiny, battleLevel);
          } else {
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
