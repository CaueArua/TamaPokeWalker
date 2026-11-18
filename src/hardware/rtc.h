#pragma once
#include <Arduino.h>
#include "../core/clock.h"

/** Initializes the PCF85063 RTC. Returns false when the chip is not detected. */
bool rtcBegin();
/** Returns whether rtcBegin() found the RTC. */
bool rtcIsPresent();
/** Reads the RTC calendar fields. Returns false when the RTC is absent. */
bool rtcReadDateTime(ClockDateTime &out);
/** Writes the RTC calendar fields. Returns false when the RTC is absent. */
bool rtcWriteDateTime(const ClockDateTime &in);
