#pragma once

#include <stdint.h>

extern bool screenOff;
extern bool powerSave;
extern uint32_t lastInteract;
extern uint8_t dimStage;

/** Loads the persisted power-saving preference. */
void loadPowerSave();

/** Updates and persists the power-saving preference. */
void setPowerSave(bool enabled);

/** Updates AMOLED brightness according to pet sleep, inactivity, and power state. */
void updateBrightness(uint32_t now);

/** Returns whether the current hardware and UI state permits light sleep. */
bool lightSleepAllowed(uint32_t now);

/** Computes a bounded light-sleep duration. */
uint16_t lightSleepMs(uint32_t now);

/** Enters light sleep for the selected duration and records touch wakeups. */
void maybeLightSleep(uint32_t now);
