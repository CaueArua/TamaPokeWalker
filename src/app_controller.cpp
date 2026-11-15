#include <Arduino.h>
#include "../audio.h"
#include "../dex.h"
#include "../i18n.h"
#include "../imu.h"
#include "../pet.h"
#include "../rtcbat.h"
#include "../sdmon.h"
#include "../species_chirp.h"
#include "../time_utils.h"
#include "app_controller.h"
#include "serial_console.h"
#include "ui/main_screen.h"
#include "ui/pet_card_page.h"
#include "ui/power_manager.h"
#include "ui/render_utils.h"
#include "ui/screen_controller.h"
#include "ui/touch_controller.h"

extern Pet pet;
extern uint8_t cardPage;
extern bool cardDirty;
extern bool gameMenuOpen;
extern uint32_t bathUntil;
extern uint32_t feedMenuUntil;
extern uint32_t confirmUntil;
extern uint8_t choiceKind;
extern uint32_t wildPromptUntil;
extern uint32_t petEventUntil;
extern uint32_t petEventFeedbackUntil;
extern char petEventMsg[18];
extern uint32_t eggWiggleUntil;
extern uint32_t lastRender;
extern uint32_t perfRenderLastMs;
extern uint32_t perfRenderMaxMs;
extern uint32_t perfRenderCount;
extern uint32_t perfLoopLastMs;
extern uint32_t perfLoopMaxMs;
extern SdMon mon;
extern PmdMon pmd;

static int16_t monFor = -2;
static bool monShinyFor = false;
static uint32_t nextAmbientSoundAt = 0;
static uint32_t lastLoopStart = 0;
/**
 * Reloads the current species sprite when its species, form, or SD state changes.
 */
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  sdDirty = false;
  monFor = pet.speciesId;
  monShinyFor = pet.shiny;
  mon.unload();
  pmd.unload();
  beh.x = beh.targetX = 233;
  beh.mode = 0;
  beh.until = 0;
  if (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    pmd.load(pet.speciesId, pet.shiny);          // Prefer the animated PMD sprite.
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // Fall back to the monochrome sprite.
  }
}

/**
 * Checks whether ambient sounds may play on the main screen.
 *
 * @return True when gameplay and display state allow ambient audio.
 */
static bool mainScreenReadyForAmbientSound() {
  if (audioMode() != SOUND_FULL || screenOff || dimStage > 0) return false;
  if (pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (activeScreen || gameMenuOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil || petEventUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}

/**
 * Plays an occasional ambient sound while the pet is idle on the main screen.
 *
 * @param now Current uptime in milliseconds.
 */
static void maybePlayAmbientSound(uint32_t now) {
  if (!mainScreenReadyForAmbientSound()) {
    nextAmbientSoundAt = now + 9000;
    return;
  }
  if (nextAmbientSoundAt == 0) nextAmbientSoundAt = now + 8000 + random(8000);
  if (deadlineActive(now, nextAmbientSoundAt)) return;
  uint8_t r = (uint8_t)random(3);
  sfxPlay(r == 0 ? SFX_HEART : (r == 1 ? SFX_EVENT_SPARKLE : SFX_MENU));
  nextAmbientSoundAt = now + 8000 + random(8000);
}

/**
 * Runs one firmware iteration: updates the pet, polls hardware, renders, and sleeps.
 */
void runAppLoop() {
  uint32_t now = millis();
  uint32_t loopStart = now;
  bool petChanged = pet.update(now);
  if (petChanged && activeScreen == petCardScreenForPage(cardPage))
    activeScreen->markDirty();

  // Play a sound when the pet becomes eligible to evolve, including after waking.
  // wantEvolveButton() already excludes pets that are still asleep.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // Play a warning when neglect makes the pet eligible to run away.
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  pollTouch();
  handleSerial();
  ensureMon();
  setPowerCacheInterval(powerSave ? 10000UL : 2000UL);
  pet.ensureDailyGoals();
  mainScreen.update(now);
  maybePlayAmbientSound(now);

  uint16_t imuMs = screenOff ? 70 : (dimStage ? 60 : 40);
  imuPoll(now, imuMs);
  uint32_t stepsBefore = pet.stepsToday;
  uint32_t stepDayBefore = pet.stepDay;
  uint16_t walked = imuTakeSteps();
  // The detector already filters isolated impacts and requires a walking rhythm.
  // Count steps immediately after USB is unplugged; cached VBUS state could
  // otherwise discard real steps for up to ten seconds.
  if (walked) {
    pet.applyWalk(walked, rtcEpoch());
  }
  mainScreen.updateWalkCards(walked, stepsBefore, stepDayBefore);
  if (imuShakeEdge() && !screenOff) {
    if (pet.isEgg()) {
      eggWiggleUntil = now + 450;
      sfxPlay(SFX_TAP);
    } else if (mainScreen.readyForShake()) {
      lastInteract = now;
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]);
      }
      if (pet.applyShake()) {
        if (audioMode() == SOUND_FULL) speciesChirpPlay(pet.speciesId, true);
        else sfxPlay(SFX_HEART);
        snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(S_HAPPY_FB));
        petEventFeedbackUntil = now + 1600;
      } else {
        sfxPlay(SFX_TAP);
      }
    }
  }

  // A short PWR press toggles the display on or off.
  static uint32_t lastPwr = 0;
  if (now - lastPwr > 250) {
    lastPwr = now;
    if (pwrShortPressed()) {
      bool wasOff = screenOff;
      screenOff = !screenOff;
      if (!screenOff) {
        lastInteract = now;
        if (wasOff) {
          suppressTouchUntil(now + 900);
        }
      }
    }
  }

  updateBrightness(now);

  // Flush periodic saves only while the display is dimmed/off or the pet sleeps.
  // NVS writes can stall both cores for about one second; at this point no
  // animation or active touch response is interrupted. Normal actions also save
  // immediately, so active gameplay remains persisted.
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }

  // Refresh the real clock periodically; the value is persisted with game saves.
  static uint32_t lastClock = 0;
  uint32_t clockPollMs = powerSave ? 60000UL : 30000UL;
  if (now - lastClock > clockPollMs) {
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) pet.lastSeenEpoch = e;
  }

  mainScreen.updateExpedition(now);

  // Emit a five-minute health heartbeat for soak testing; ignore it without a monitor.
  static uint32_t lastHealth = 0;
  uint32_t healthMs = powerSave ? 600000UL : 300000UL;
  if (now - lastHealth > healthMs) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u rMax=%lums lMax=%lums screen=%s\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  (unsigned long)perfRenderMaxMs, (unsigned long)perfLoopMaxMs, screenName());
    perfRenderMaxMs = 0;
    perfLoopMaxMs = 0;
  }

  // Use conservative game and battle cadences so rendering does not overlap
  // the previous DMA transfer. Clean static screens skip rendering.
  if (now - lastRender >= renderIntervalMs()) {
    lastRender = now;
    uint32_t rt0 = millis();
    render();
    perfRenderLastMs = millis() - rt0;
    if (perfRenderLastMs > perfRenderMaxMs) perfRenderMaxMs = perfRenderLastMs;
    perfRenderCount++;
  }

  maybeLightSleep(millis());
  perfLoopLastMs = millis() - loopStart;
  if (perfLoopLastMs > perfLoopMaxMs) perfLoopMaxMs = perfLoopLastMs;
  lastLoopStart = loopStart;
}

