#pragma once

#include <stdint.h>
#include "../../audio.h"

inline uint8_t settingsVolumeLevel(uint8_t mode) {
  if (mode == SOUND_OFF) return 0;
  return mode == SOUND_FULL ? 2 : 1;
}

inline uint8_t settingsVolumeMode(uint8_t mode, int direction) {
  if (mode == SOUND_LOW) return direction > 0 ? SOUND_MED : SOUND_OFF;
  int level = settingsVolumeLevel(mode) + direction;
  if (level < 0) level = 0;
  if (level > 2) level = 2;
  return level == 0 ? SOUND_OFF : level == 1 ? SOUND_MED : SOUND_FULL;
}

inline uint8_t settingsBrightnessStep(uint8_t value, int direction) {
  int next = value + direction * 10;
  if (next < 0) next = 0;
  if (next > 100) next = 100;
  return (uint8_t)next;
}
