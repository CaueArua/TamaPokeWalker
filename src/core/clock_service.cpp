#include "clock_service.h"
#include <Arduino.h>
#include "../hardware/rtc.h"
#include "../hardware/soft_clock_store.h"

namespace {
constexpr uint32_t SAVE_INTERVAL_MS = 60000;

// Software clock: epoch seen at baseMillis, advanced by millis() while powered.
uint32_t baseEpoch = 0;
uint32_t baseMillis = 0;
bool baseValid = false;
uint32_t lastTickMs = 0;

bool epochInRange(uint32_t epoch) {
  ClockDateTime date = clockDateTime(epoch);
  return date.year >= 2025 && date.year <= 2120;
}

void setBase(uint32_t epoch) {
  baseEpoch = epoch;
  baseMillis = millis();
  baseValid = true;
}

uint32_t softNow() {
  if (!baseValid) return 0;
  return baseEpoch + (millis() - baseMillis) / 1000;
}

bool rtcNow(uint32_t &epoch) {
  ClockDateTime date;
  if (!rtcReadDateTime(date)) return false;
  if (date.year < 2025 || date.year > 2120) return false;  // Invalid RTC time.
  epoch = epochFromDateTime(date);
  return true;
}
}  // namespace

namespace Clock {

bool begin() {
  bool rtcFound = rtcBegin();
  uint32_t saved = 0;
  if (softClockStoreLoad(saved) && epochInRange(saved)) setBase(saved);
  return rtcFound;
}

bool available() { return rtcIsPresent(); }

uint32_t nowEpoch() {
  uint32_t epoch = 0;
  if (rtcNow(epoch)) return epoch;
  return softNow();
}

void set(uint32_t epoch) {
  if (rtcIsPresent()) rtcWriteDateTime(clockDateTime(epoch));
  setBase(epoch);
  softClockStoreSave(epoch);
  lastTickMs = millis();
}

void tick() {
  uint32_t now = millis();
  if (now - lastTickMs < SAVE_INTERVAL_MS) return;
  lastTickMs = now;

  uint32_t rtcEpoch = 0;
  if (rtcNow(rtcEpoch)) {
    setBase(rtcEpoch);  // Keep the fallback close to the RTC without writing flash.
    return;
  }
  if (baseValid) softClockStoreSave(softNow());
}

}  // namespace Clock

