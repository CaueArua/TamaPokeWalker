#include "startup.h"

#include <Arduino.h>

#include "../audio.h"
#include "../pet.h"
#include "../sdmon.h"
#include "app_controller.h"
#include "core/clock_service.h"
#include "hardware/power.h"
#include "ui/clock_screen.h"
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
  bool awaitingStarter = pet.awaitingStarter();
  Serial.printf("SAVE %s spec=%d lv=%u wins=%u losses=%u streak=%u\n",
                pet.saveLoadedFromNvs ? "loaded" : "created",
                pet.speciesId, pet.level(), pet.battleWins, pet.battleLosses, pet.battleStreak);
  thumbs.load();

  pwrSetup();
  uint32_t e = Clock::nowEpoch();
  if (e == 0) {
    // RTC absent, stopped, or not set: the user must set the time before the pet can sync.
    clockSettingsScreen.openAtBoot(awaitingStarter ? &starterScreen : nullptr);
    Serial.println("RTC sem hora: abrindo configuracao de data e hora");
  } else {
    // Synchronize the pet with the real-time clock, including powered-off time.
    pet.syncClock(e);
    if (awaitingStarter) setScreen(&starterScreen);
  }
  loadPowerSave();

  ensureMon();
  render();
  lastRender = millis();

  audioBegin();  // Initialize ES8311, I2S, and the amplifier; play the startup jingle.

  lastInteract = millis();
}
