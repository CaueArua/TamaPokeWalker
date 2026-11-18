#include "soft_clock_store.h"
#include <Preferences.h>

namespace {
// Shares the "tamapoke" namespace with the other saved settings; this key is unique to the clock.
constexpr const char *NAMESPACE = "tamapoke";
constexpr const char *KEY = "clkSoft";
}  // namespace

bool softClockStoreLoad(uint32_t &epoch) {
  Preferences prefs;
  prefs.begin(NAMESPACE, true);
  epoch = prefs.getULong(KEY, 0);
  prefs.end();
  return epoch != 0;
}

void softClockStoreSave(uint32_t epoch) {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.putULong(KEY, epoch);
  prefs.end();
}
