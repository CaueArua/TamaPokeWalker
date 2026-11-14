#pragma once

#include <stdint.h>

/** Returns the scene hour from an RTC Unix timestamp, defaulting to 13 if unset. */
inline int sceneHourFromEpoch(uint32_t epoch) {
  return epoch ? (int)((epoch / 3600UL) % 24UL) : 13;
}

/** Returns the UTC day number represented by a Unix timestamp, or zero if unset. */
inline uint32_t unixDayFromEpoch(uint32_t epoch) {
  return epoch ? epoch / 86400UL : 0;
}

/** Returns the UTC hour number represented by a Unix timestamp, or zero if unset. */
inline uint32_t unixHourFromEpoch(uint32_t epoch) {
  return epoch ? epoch / 3600UL : 0;
}

/** Maps an hour of day to morning, day, evening, or night. */
inline uint8_t dayPhaseFromHour(int hour) {
  if (hour >= 6 && hour < 12) return 0;
  if (hour >= 12 && hour < 18) return 1;
  if (hour >= 18 && hour < 22) return 2;
  return 3;
}

/** Maps a Unix timestamp to its day phase. */
inline uint8_t dayPhaseFromEpoch(uint32_t epoch) {
  return dayPhaseFromHour(sceneHourFromEpoch(epoch));
}

/** Returns whether a day-phase value represents night. */
inline bool isNightPhase(uint8_t phase) {
  return phase == 3;
}

/** Returns whether the scene should use nighttime visuals. */
inline bool isVisualNight(int hour, bool sleeping) {
  return sleeping || hour < 6 || hour >= 20;
}

/** Returns the food-decay amount for the current day phase. */
inline uint8_t nightFoodDrop(uint32_t epoch) {
  return isNightPhase(dayPhaseFromEpoch(epoch)) ? 1 : 2;
}
