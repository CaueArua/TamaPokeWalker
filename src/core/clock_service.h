#pragma once
#include <stdint.h>
#include "clock.h"

// Single entry point for wall-clock time. Hardware access stays in src/hardware.
namespace Clock {
/**
 * Initializes the RTC and restores the last saved software-clock time from NVS.
 * Returns false when the RTC is not detected.
 */
bool begin();
/** Returns whether the RTC hardware is present. */
bool available();
/**
 * Returns the current time as Unix seconds, or zero if unknown.
 * Uses the RTC when it holds a valid time, otherwise the software clock.
 */
uint32_t nowEpoch();
/** Sets the time in the software clock and in the RTC when it is present. */
void set(uint32_t epoch);
/**
 * Keeps the software clock aligned and, while the RTC is not usable, saves it to NVS once per minute.
 * Call from the main loop.
 */
void tick();
}
