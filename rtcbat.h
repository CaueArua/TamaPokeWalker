#pragma once
#include <Arduino.h>

/** Initializes the PCF85063 RTC. */
bool rtcBegin();
/** Returns the RTC time as Unix seconds, or zero if the clock is invalid. */
uint32_t rtcEpoch();
/** Sets the RTC to a Unix timestamp. */
void rtcSetEpoch(uint32_t e);

/** Initializes the AXP2101 power-management unit. */
bool batBegin();
/** Enables BLDO1 for the OLED 3.3 V rail; call before gfx->begin(). */
void pmuEnablePanel();
/** Returns battery charge from 0 to 100, or -1 when no battery is connected. */
int batPercent();
/** Returns whether the battery is charging. */
bool batCharging();
/** Returns whether USB power is present. */
bool usbPresent();
/** Sets how long cached power data remains valid. */
void setPowerCacheInterval(uint32_t ms);

/** Configures the AXP2101 power button. */
void pwrSetup();
/** Polls for and consumes a short power-button press. */
bool pwrShortPressed();
