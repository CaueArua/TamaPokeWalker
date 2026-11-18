#include "power_manager.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include <esp_sleep.h>

#include "../../audio.h"
#include "../hardware/power.h"
#include "../../imu.h"
#include "../../pet.h"
#include "../../pin_config.h"
#include "../../time_utils.h"
#include "screen.h"
#include "screen_controller.h"
#include "touch_controller.h"

extern Arduino_CO5300 *panel;
extern Pet pet;
extern bool gameMenuOpen;
extern uint32_t bathUntil;
extern uint32_t feedMenuUntil;
extern uint32_t confirmUntil;
extern uint8_t choiceKind;
extern uint32_t wildPromptUntil;
extern uint32_t petEventUntil;
extern uint32_t lastRender;

bool screenOff = false;
bool powerSave = false;
uint32_t lastInteract = 0;
uint8_t dimStage = 0;

void loadPowerSave() {
  Preferences preferences;
  preferences.begin("tamapoke", true);
  powerSave = preferences.getBool("psave", false);
  preferences.end();
}

void setPowerSave(bool enabled) {
  powerSave = enabled;
  Preferences preferences;
  preferences.begin("tamapoke", false);
  preferences.putBool("psave", powerSave);
  preferences.end();
}

bool lightSleepAllowed(uint32_t now) {
  if (!powerSave || usbPresent() || audioBusy() || Serial.available()) return false;
  if (!screenOff && dimStage == 0) return false;
  if (!screenOff && now - lastInteract < 1500UL) return false;
  if (touchInputBusy(now)) return false;
  if (activeScreen || bathUntil) return false;
  if (pet.awaitingStarter() || feedMenuUntil || confirmUntil || choiceKind ||
      wildPromptUntil || petEventUntil) return false;
  if (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart()) return false;
  if (gameMenuOpen) return false;
  return true;
}

uint16_t lightSleepMs(uint32_t now) {
  if (!lightSleepAllowed(now)) return 0;
  uint16_t maxMs = screenOff ? 750 : (dimStage >= 2 ? 300 : 180);
  if (imuOk() && maxMs > 70) maxMs = 70;
  uint16_t intervalMs = renderIntervalMs();
  uint32_t sinceRender = now - lastRender;
  if (sinceRender < intervalMs) {
    uint32_t untilRender = intervalMs - sinceRender;
    if (untilRender < maxMs) maxMs = untilRender;
  }
  if (maxMs < 40) return 0;
  return maxMs;
}

void maybeLightSleep(uint32_t now) {
  uint16_t sleepMs = lightSleepMs(now);
  if (!sleepMs) return;

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TP_INT, 0);
  esp_light_sleep_start();

  if (digitalRead(TP_INT) == LOW || esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    notifyTouchWake();
  }
}

void updateBrightness(uint32_t now) {
  if (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart()) {
    lastInteract = now;
  }
  uint32_t idleMs = now - lastInteract;
  dimStage = (idleMs > 300000) ? 2 : (idleMs > 90000) ? 1 : 0;
  uint8_t target = pet.sleeping ? 25 : (usbPresent() ? 180 : 145);
  if (dimStage == 1) target = pet.sleeping ? 10 : 60;
  else if (dimStage == 2) target = 8;
  if (screenOff) target = 0;
  static uint8_t current = 255;
  if (target != current) {
    current = target;
    panel->setBrightness(target);
  }
}
