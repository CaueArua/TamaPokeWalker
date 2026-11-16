#include <Arduino.h>
#include <stdio.h>
#include "../audio.h"
#include "../biome.h"
#include "../dex.h"
#include "../game_info.h"
#include "../imu.h"
#include "../pet.h"
#include "../rtcbat.h"
#include "../sdmon.h"
#include "ui/battle_screen.h"
#include "ui/dex_screen.h"
#include "ui/screen_controller.h"

extern Pet pet;
extern bool uiDirty;
extern bool cardDirty;
extern bool clockDirty;
extern bool helpDirty;
extern bool keyboardDirty;
extern bool gameMenuDirty;
extern uint32_t perfRenderLastMs;
extern uint32_t perfRenderMaxMs;
extern uint32_t perfRenderCount;
extern uint32_t perfRenderSkipCount;
extern uint32_t perfLoopLastMs;
extern uint32_t perfLoopMaxMs;
extern SdMon mon;
extern PmdMon pmd;

void markUiDirty();
const char *firmwareVersion();
int sceneHour();
uint8_t currentDayPhase();
// Serial console for SD provisioning, save backups, diagnostics, and local testing.

/**
 * Reads and dispatches one serial command or SD transfer request.
 */
void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;

  // Browser save-backup protocol:
  // SAVEGET -> "SAVE <bytes>" + hex data + DONE.
  // SAVEPUT <bytes> -> OK + raw data -> DONE, then reboot. Pet validates the
  // checksum and rejects altered or incomplete snapshots before updating NVS.
  if (line == "SAVEGET") {
    uint8_t backup[Pet::BACKUP_MAX_BYTES];
    size_t size = pet.exportBackup(backup, sizeof(backup));
    if (!size) { Serial.println("ERR"); return; }
    // Normal game logs must not block when no terminal is connected, so setup
    // uses a zero TX timeout. A connected browser explicitly requests backups;
    // wait briefly for the complete hex response instead of dropping its data.
    Serial.setTxTimeoutMs(1500);
    Serial.printf("SAVE %u\n", (unsigned)size);
    Serial.print("SAVEHEX ");
    static const char HEX_CHARS[] = "0123456789ABCDEF";
    for (size_t i = 0; i < size; ++i) { Serial.print(HEX_CHARS[backup[i] >> 4]); Serial.print(HEX_CHARS[backup[i] & 15]); }
    Serial.println();
    Serial.println("DONE");
    Serial.flush();
    Serial.setTxTimeoutMs(0);
    return;
  }
  if (line.startsWith("SAVEPUT ")) {
    uint32_t size = (uint32_t)line.substring(8).toInt();
    if (size == 0 || size > Pet::BACKUP_MAX_BYTES) { Serial.println("ERR"); return; }
    uint8_t backup[Pet::BACKUP_MAX_BYTES];
    Serial.println("OK");
    Serial.setTimeout(6000);
    size_t got = Serial.readBytes(backup, size);
    Serial.setTimeout(1000);
    if (got != size || !pet.importBackup(backup, size)) { Serial.println("ERR"); return; }
    Serial.println("DONE");
    delay(150);
    ESP.restart();
    return;
  }

  if (line == "PERF") {
    Serial.printf("screen=%s render=%lums max=%lums count=%lu skip=%lu loop=%lums loopMax=%lums interval=%u dirty ui=%d card=%d clock=%d help=%d kb=%d menu=%d battle=%d gallery=%d\n",
                  screenName(),
                  (unsigned long)perfRenderLastMs, (unsigned long)perfRenderMaxMs,
                  (unsigned long)perfRenderCount, (unsigned long)perfRenderSkipCount,
                  (unsigned long)perfLoopLastMs, (unsigned long)perfLoopMaxMs,
                  renderIntervalMs(), uiDirty, cardDirty, clockDirty, helpDirty,
                  keyboardDirty, gameMenuDirty, battleScreen.isDirty(), dexScreen.isDirty());
    perfRenderMaxMs = 0;
    perfLoopMaxMs = 0;
    Serial.println("DONE");
  } else if (line == "HATCH") {
    pet.eggTap(); pet.eggTap(); pet.eggTap();
    Serial.println("DONE");
  } else if (line.startsWith("SPEC ")) {
    int n = line.substring(5).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      Serial.printf("especie #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("LVL ")) {
    pet.ageMinutes = (uint32_t)line.substring(4).toInt() * MINUTES_PER_LEVEL;
    Serial.println("DONE");
  } else if (line.startsWith("TIME ")) {
    uint32_t e = (uint32_t)line.substring(5).toInt();
    rtcSetEpoch(e);
    pet.setClock(e);
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("RTCSET ")) {  // Set only the RTC to simulate powered-off time.
    rtcSetEpoch((uint32_t)line.substring(7).toInt());
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "TIME") {
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "GAL") {
    if (activeScreen == &dexScreen) {
      setScreen(nullptr);
    } else {
      dexScreen.openGrid();
      setScreen(&dexScreen);
    }
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // Simulate 20 egg-species rolls without changing game state.
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // Converts and persists the active pet as shiny.
    if (!pet.setActiveShiny()) {
      Serial.println("ERR nao e possivel converter um ovo");
    } else {
      markUiDirty();
      Serial.printf("shiny=%d\n", pet.shiny);
    }
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // Simulate a new day of care for testing.
    pet.setClock(pet.lastSeenEpoch + 86400);
    pet.caress();
    Serial.printf("streak=%u bond=%u medals=0x%X\n", pet.streak, pet.bond, pet.medals);
    Serial.println("DONE");
  } else if (line == "BYE") {
    pet.startFarewell();
    Serial.println("DONE");
  } else if (line == "RUN") {
    pet.startRunaway();
    Serial.println("DONE");
  } else if (line == "BEEP") {
    sfxPlay(SFX_HATCH);  // Test audio playback.
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // Force runaway readiness to test its prompt.
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // Erase NVS and reboot into a new starter selection.
    Serial.println("DONE");
    delay(100);
    ESP.restart();
  } else if (line == "REG") {
    Serial.printf("pokedex %u/%u:", pet.registeredCount(), DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isRegistered(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
#ifdef TAMAPOKE_LOCAL_TEST
  } else if (line.startsWith("TESTMON ")) {
    int split = line.indexOf(' ', 8);
    int n = line.substring(8, split < 0 ? line.length() : split).toInt();
    int shinyFlag = split < 0 ? 0 : line.substring(split + 1).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      pet.shiny = shinyFlag != 0;
      pet.sleeping = false;
      pet.ceremony = CER_NONE;
      markUiDirty();
      Serial.printf("testmon #%d %s shiny=%d\n", n, DEX_TBL[n].name, pet.shiny);
    }
    Serial.println("DONE");
  } else if (line.startsWith("TESTEVO ")) {
    int split = line.indexOf(' ', 8);
    int source = line.substring(8, split < 0 ? line.length() : split).toInt();
    int target = split < 0 ? 0 : line.substring(split + 1).toInt();
    if (source >= 1 && source <= DEX_COUNT) {
      pet.speciesId = source;
      pet.prevSpeciesId = -1;
      pet.sleeping = false;
      pet.ceremony = CER_NONE;
      pet.resetEvolutionDeferral();
      pet.careMistakes = 0;
      uint8_t requiredLevel = pet.evolutionRequiredLevelFor(target);
      pet.ageMinutes = (uint32_t)(requiredLevel > 0 ? requiredLevel - 1 : 0) * MINUTES_PER_LEVEL;
      pet.fullness = pet.joy = pet.energy = pet.hygiene = 80;
      pet.bond = 60;
      pet.evs[STAT_ATK] = pet.evs[STAT_DEF] = pet.evs[STAT_SPE] = 0;
      uint32_t testEpoch = 1767225600UL + 12UL * 3600UL;
      if (target == 197) testEpoch += 9UL * 3600UL;
      pet.setClock(testEpoch);
      if (source == 236) {
        if (target == 106) pet.evs[STAT_ATK] = 80;
        else if (target == 107) pet.evs[STAT_DEF] = 80;
        else pet.evs[STAT_ATK] = pet.evs[STAT_DEF] = 40;
      }
      markUiDirty();
      Serial.printf("testevo #%d -> #%d level=%u bond=%u\n", source, target,
                    pet.level(), pet.bond);
    }
    Serial.println("DONE");
#endif
  } else if (line.startsWith("GAMEINFO ")) {
    // Format: GAMEINFO nome|id|regiao|rota (the pipe separates fields, so names may contain spaces).
    String args = line.substring(9);
    int a = args.indexOf('|');
    int b = a < 0 ? -1 : args.indexOf('|', a + 1);
    int c = b < 0 ? -1 : args.indexOf('|', b + 1);
    long id = c < 0 ? -1 : args.substring(a + 1, b).toInt();
    if (c < 0 || id < 0 || id > 999999) {
      Serial.println("ERR uso: GAMEINFO nome|id|regiao|rota (id 0-999999)");
    } else {
      gameInfoSetPlayer(args.substring(0, a).c_str(), (uint32_t)id,
                        args.substring(b + 1, c).c_str(), args.substring(c + 1).c_str());
      Serial.printf("nome=%s id=%u regiao=%s rota=%s\n", gameInfo.playerName,
                    gameInfo.playerId, gameInfo.region, gameInfo.route);
      Serial.println("DONE");
    }
  } else if (line.startsWith("BIOME ")) {
    // Format: BIOME id (0 to BIOME_COUNT - 1); changes only the biome.
    long id = line.substring(6).toInt();
    if (id < 0 || id >= BIOME_COUNT || !gameInfoSetBiome((uint8_t)id)) {
      Serial.printf("ERR uso: BIOME id (0-%d)\n", BIOME_COUNT - 1);
    } else {
      Serial.printf("bioma=%u\n", gameInfo.biome);
      Serial.println("DONE");
    }
  } else if (line == "AVAILCLEAR") {
    gameInfoClearAvailable();
    Serial.println("DONE");
  } else if (line.startsWith("AVAILADD ")) {
    // Format: AVAILADD dex forma raridade passos
    unsigned dex = 0, form = 0, rarity = 0, steps = 0;
    if (sscanf(line.c_str(), "AVAILADD %u %u %u %u", &dex, &form, &rarity, &steps) != 4 ||
        dex < 1 || dex > DEX_COUNT || form > 255 || rarity > 255) {
      Serial.println("ERR uso: AVAILADD dex forma raridade passos");
    } else {
      int8_t slot = gameInfoAddAvailable((uint16_t)dex, (uint8_t)form, (uint8_t)rarity, steps);
      if (slot < 0) {
        Serial.println("ERR lista cheia (30 posicoes); use AVAILCLEAR");
      } else {
        Serial.printf("available[%d] = #%u %s forma=%u raridade=%u passos=%u\n", slot, dex,
                      DEX_TBL[dex].name, form, rarity, steps);
        Serial.println("DONE");
      }
    }
  } else if (line.startsWith("IVSET ")) {
    // Format: IVSET alvo nature iv0 iv1 iv2 iv3 iv4 iv5 (alvo: -1 all, 0 active, 1-6 party slot).
    int target = 0;
    unsigned nat = 0, ivv[STAT_COUNT] = { 0 };
    if (sscanf(line.c_str(), "IVSET %d %u %u %u %u %u %u %u", &target, &nat, &ivv[0], &ivv[1],
               &ivv[2], &ivv[3], &ivv[4], &ivv[5]) != 8 ||
        target < -1 || target > PARTY_SIZE || nat >= NATURE_COUNT) {
      Serial.printf("ERR uso: IVSET alvo(-1|0-%d) nature(0-%d) iv0..iv5(0-31)\n", PARTY_SIZE, NATURE_COUNT - 1);
    } else {
      uint8_t newIvs[STAT_COUNT];
      bool ok = true;
      for (uint8_t i = 0; i < STAT_COUNT; i++) {
        if (ivv[i] > 31) ok = false;
        newIvs[i] = (uint8_t)ivv[i];
      }
      if (!ok) {
        Serial.println("ERR IV fora de 0-31");
      } else {
        int8_t slot = target < 0 ? -1 : (target == 0 ? (int8_t)pet.activePartySlot : (int8_t)(target - 1));
        int changed = pet.setPartyStats(slot, newIvs, (uint8_t)nat);
        if (changed == 0) {
          Serial.println("ERR nenhum pokemon alterado (slot vazio ou ovo)");
        } else {
          Serial.printf("alterados=%d nature=%s ivs=%u/%u/%u/%u/%u/%u\n", changed, natureName((uint8_t)nat),
                        newIvs[0], newIvs[1], newIvs[2], newIvs[3], newIvs[4], newIvs[5]);
          Serial.println("DONE");
        }
      }
    }
  } else if (line.startsWith("EVSET ")) {
    // Format: EVSET ev0 ev1 ev2 ev3 ev4 ev5 (active pet; each 0-252, total <= 510).
    unsigned evv[STAT_COUNT] = { 0 };
    if (sscanf(line.c_str(), "EVSET %u %u %u %u %u %u", &evv[0], &evv[1], &evv[2], &evv[3], &evv[4],
               &evv[5]) != 6) {
      Serial.println("ERR uso: EVSET ev0..ev5 (0-252 cada, total 0-510)");
    } else {
      uint8_t newEvs[STAT_COUNT];
      for (uint8_t i = 0; i < STAT_COUNT; i++) newEvs[i] = (uint8_t)(evv[i] > 255 ? 255 : evv[i]);
      if (!pet.setActiveEvs(newEvs)) {
        Serial.println("ERR EV fora dos limites (0-252 cada, total 510) ou ovo");
      } else {
        markUiDirty();
        Serial.printf("evs=%u/%u/%u/%u/%u/%u\n", pet.evs[0], pet.evs[1], pet.evs[2], pet.evs[3], pet.evs[4],
                      pet.evs[5]);
        Serial.println("DONE");
      }
    }
  } else if (line.startsWith("SPECIES ")) {
    // Format: SPECIES natNum (National Dex number); maps it to the DEX_TBL index and redraws the screen.
    long natNum = line.substring(8).toInt();
    int dex = 0;
    for (int i = 1; i <= DEX_COUNT && natNum > 0; i++) {
      if (DEX_TBL[i].natNum == natNum) {
        dex = i;
        break;
      }
    }
    if (dex == 0) {
      Serial.println("ERR natNum nao encontrado em DEX_TBL");
    } else if (!pet.setActiveSpecies((int16_t)dex)) {
      Serial.println("ERR nao foi possivel alterar (ovo)");
    } else {
      markUiDirty();
      Serial.printf("natNum=%ld speciesId=%d %s\n", natNum, dex, DEX_TBL[dex].name);
      Serial.println("DONE");
    }
  } else if (line.startsWith("LEVEL ")) {
    // Format: LEVEL nivel (1-100); sets the active pet's level and redraws the screen.
    long lv = line.substring(6).toInt();
    if (lv < 1 || lv > 100) {
      Serial.println("ERR uso: LEVEL nivel(1-100)");
    } else if (!pet.setActiveLevel((uint8_t)lv)) {
      Serial.println("ERR nao foi possivel alterar (ovo)");
    } else {
      markUiDirty();
      Serial.printf("level=%u\n", pet.level());
      Serial.println("DONE");
    }
  } else if (line.startsWith("STAT ")) {
    // Format: STAT HP|ATK|DEF|SPA|SPD|SPE (active pet); prints base, IV, EV, level, nature effect and final value.
    static const char *const kStatNames[STAT_COUNT] = { "HP", "ATK", "DEF", "SPA", "SPD", "SPE" };
    String arg = line.substring(5);
    arg.trim();
    arg.toUpperCase();
    int idx = -1;
    for (uint8_t i = 0; i < STAT_COUNT; i++)
      if (arg == kStatNames[i]) idx = i;
    StatBreakdown b;
    if (idx < 0) {
      Serial.println("ERR uso: STAT HP|ATK|DEF|SPA|SPD|SPE");
    } else if (!pet.statBreakdown((uint8_t)idx, b)) {
      Serial.println("ERR nao disponivel (ovo)");
    } else {
      const char *efeito = b.natureMod > 0 ? "sobe" : (b.natureMod < 0 ? "desce" : "neutra");
      Serial.printf("%s base=%u iv=%u ev=%u level=%u nature=%s(%s) final=%u\n", kStatNames[idx], b.base, b.iv,
                    b.ev, b.level, natureName(pet.nature), efeito, b.finalValue);
      Serial.println("DONE");
    }
  } else if (line.startsWith("BOND ")) {
    // Format: BOND valor (0-255); sets the active pet's affection and redraws the screen.
    long value = line.substring(5).toInt();
    if (value < 0 || value > 255) {
      Serial.println("ERR uso: BOND valor(0-255)");
    } else {
      pet.setActiveBond((uint8_t)value);
      markUiDirty();
      Serial.printf("bond=%u\n", pet.bond);
      Serial.println("DONE");
    }
#ifdef TAMAPOKE_LOCAL_TEST
  } else if (line == "CAUGHT") {
    Serial.printf("caught %u/%u:", pet.caughtCount(), DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isCaught(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
  } else if (line.startsWith("CAUGHT ")) {
    int n = line.substring(7).toInt();
    if (n < 1 || n > DEX_COUNT) n = pet.speciesId;
    if (n >= 1 && n <= DEX_COUNT) {
      pet.registerCaught((int16_t)n);
      Serial.printf("caught #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("BATTLE ")) {
    int split = line.indexOf(' ', 7);
    int n = line.substring(7, split < 0 ? line.length() : split).toInt();
    int8_t shinyOverride = -1;
    if (split >= 0) shinyOverride = line.substring(split + 1).toInt() ? 1 : 0;
    battleScreen.start((int16_t)n, pet.level(), shinyOverride);
    int16_t opponent = battleScreen.opponentDex();
    Serial.printf("battle #%d %s shiny=%d caught=%d\n", opponent,
                  (opponent >= 1 && opponent <= DEX_COUNT) ? DEX_TBL[opponent].name : "?",
                  battleScreen.opponentShiny(), pet.isCaught(opponent));
    Serial.println("DONE");
  } else if (line == "BATTLE") {
    battleScreen.start();
    int16_t opponent = battleScreen.opponentDex();
    Serial.printf("battle #%d %s caught=%d\n", opponent,
                  (opponent >= 1 && opponent <= DEX_COUNT) ? DEX_TBL[opponent].name : "?",
                  pet.isCaught(opponent));
    Serial.println("DONE");
#endif
  } else if (line == "SAVEINFO") {
    Serial.printf("fw=%s save=%s createdBoot=%d spec=%d level=%u egg=%d starter=%d age=%lu\n",
                  firmwareVersion(), pet.saveLoadedFromNvs ? "loaded" : "created",
                  pet.saveCreatedThisBoot, pet.speciesId, pet.level(), pet.isEgg(),
                  pet.awaitingStarter(), (unsigned long)pet.ageMinutes);
    Serial.printf("battle wins=%u losses=%u streak=%u best=%u nick=%s\n",
                  pet.battleWins, pet.battleLosses, pet.battleStreak,
                  pet.bestBattleStreak, pet.nick);
    Serial.printf("records ball=%u catch=%u memo=%u sack=%u\n",
                  pet.gameHi, pet.catchHi, pet.memoHi, pet.strHi);
    Serial.println("DONE");
  } else if (line == "HEALTH") {
    Serial.printf("up=%lus heap=%u min=%u sd=%d mon=%d\n",
                  (unsigned long)(millis() / 1000), ESP.getFreeHeap(),
                  ESP.getMinFreeHeap(), sdReady, pmd.loaded || mon.loaded);
    Serial.println("DONE");
  } else if (line == "STATS") {
    Serial.printf("spec=%d nv=%u com=%u fel=%u ene=%u lim=%u desc=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("peso=%u fue=%u def=%u vel=%u nat=%s ivs=%u/%u/%u/%u/%u/%u evs=%u/%u/%u/%u/%u/%u baya=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  natureName(pet.nature),
                  pet.ivs[0], pet.ivs[1], pet.ivs[2], pet.ivs[3], pet.ivs[4], pet.ivs[5],
                  pet.evs[0], pet.evs[1], pet.evs[2], pet.evs[3], pet.evs[4], pet.evs[5],
                  pet.berryKnown);
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.printf("steps=%lu/%lu trail=%u shinyLuck=%u/4096 catchBonus=%u\n",
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.stepTrailRank(), pet.stepShinyChancePer4096(), pet.stepCatchBonus());
    Serial.println("DONE");
  } else if (line == "IMU") {
    Serial.printf("imu=%d addr=0x%02X mag=%.2f gyro=%.0f rawPed=%lu softPed=%lu phase=%u hour=%d\n",
                  imuOk(), imuAddr(), imuLastMagG(), imuLastGyroDps(),
                  (unsigned long)imuPedometer(), (unsigned long)imuSoftwarePedometer(),
                  currentDayPhase(), sceneHour());
    Serial.println("DONE");
  } else if (line == "SHAKE") {
    bool okShake = pet.applyShake();
    Serial.printf("shake=%d joy=%u\n", okShake, pet.joy);
    Serial.println("DONE");
  } else if (line.startsWith("WALK ")) {
    uint16_t n = (uint16_t)line.substring(5).toInt();
    uint8_t gained = pet.applyWalk(n, rtcEpoch());
    Serial.printf("walk=%u today=%lu total=%lu joy=%u bond=%u\n", gained,
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.joy, pet.bond);
    Serial.println("DONE");
  } else if (line == "STEPS") {
    Serial.printf("steps today=%lu total=%lu rank=%u shiny=%u/4096 catch+%u usb=%d imu=%d raw=%lu soft=%lu\n",
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.stepTrailRank(), pet.stepShinyChancePer4096(), pet.stepCatchBonus(),
                  usbPresent(), imuOk(), (unsigned long)imuPedometer(),
                  (unsigned long)imuSoftwarePedometer());
    Serial.println("DONE");
  }
}

