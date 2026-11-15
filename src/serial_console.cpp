#include <Arduino.h>
#include "../audio.h"
#include "../dex.h"
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
  } else if (line == "SHINY") {  // Toggle the current pet's shiny state for testing.
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
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
      pet.trAtk = pet.trDef = pet.trSpe = 0;
      uint32_t testEpoch = 1767225600UL + 12UL * 3600UL;
      if (target == 197) testEpoch += 9UL * 3600UL;
      pet.setClock(testEpoch);
      if (source == 236) {
        if (target == 106) pet.trAtk = 80;
        else if (target == 107) pet.trDef = 80;
        else pet.trAtk = pet.trDef = 40;
      }
      markUiDirty();
      Serial.printf("testevo #%d -> #%d level=%u bond=%u\n", source, target,
                    pet.level(), pet.bond);
    }
    Serial.println("DONE");
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
    Serial.printf("peso=%u fue=%u def=%u vel=%u genes=%u/%u/%u tr=%u/%u/%u baya=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe, pet.berryKnown);
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

