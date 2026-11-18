#pragma once
#include <stdint.h>

/** Loads the last epoch saved by the software clock. Returns false when none is stored. */
bool softClockStoreLoad(uint32_t &epoch);
/** Saves the software clock epoch to NVS. */
void softClockStoreSave(uint32_t epoch);
