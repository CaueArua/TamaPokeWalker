#include "startup.h"

#include <Arduino.h>

#include "../audio.h"
#include "../pet.h"
#include "../rtcbat.h"
#include "../sdmon.h"
#include "app_controller.h"
#include "ui/power_manager.h"
#include "ui/screen_controller.h"
#include "ui/starter_screen.h"
#include "../game_info.h"

extern Pet pet;
extern uint32_t lastRender;

/** Initializes game state, storage, power services, and the user interface. */
void initializeProgram() {
  pet.begin();
  gameInfoLoad();
  if (pet.awaitingStarter()) setScreen(&starterScreen);
  Serial.printf("SAVE %s spec=%d lv=%u wins=%u losses=%u streak=%u\n",
                pet.saveLoadedFromNvs ? "loaded" : "created",
                pet.speciesId, pet.level(), pet.battleWins, pet.battleLosses, pet.battleStreak);
  thumbs.load();

  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    rtcSetEpoch(1767225600UL);  // Seed a blank RTC; only elapsed-time differences matter.
    e = rtcEpoch();
    Serial.println("RTC sin hora: sembrado, sin progresion offline esta vez");
  }
  // Synchronize the pet with the real-time clock, including powered-off time.
  pet.syncClock(e);
  loadPowerSave();

  ensureMon();
  render();
  lastRender = millis();

  audioBegin();  // Initialize ES8311, I2S, and the amplifier; play the startup jingle.

  lastInteract = millis();
}
