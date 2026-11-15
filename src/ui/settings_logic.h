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

struct SettingsDateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
};

inline bool settingsLeapYear(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline int settingsDaysInMonth(int year, int month) {
  static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return days[month - 1] + (month == 2 && settingsLeapYear(year) ? 1 : 0);
}

inline SettingsDateTime settingsDateTimeFromEpoch(uint32_t epoch) {
  uint32_t days = epoch / 86400UL;
  int year = 1970;
  while (days >= (uint32_t)(settingsLeapYear(year) ? 366 : 365)) {
    days -= settingsLeapYear(year) ? 366 : 365;
    ++year;
  }
  int month = 1;
  while (days >= (uint32_t)settingsDaysInMonth(year, month)) {
    days -= settingsDaysInMonth(year, month);
    ++month;
  }
  return { year, month, (int)days + 1, (int)((epoch / 3600UL) % 24),
           (int)((epoch / 60UL) % 60) };
}

inline uint32_t settingsDateTimeEpoch(const SettingsDateTime &date) {
  uint32_t days = 0;
  for (int year = 1970; year < date.year; ++year) days += settingsLeapYear(year) ? 366 : 365;
  for (int month = 1; month < date.month; ++month) days += settingsDaysInMonth(date.year, month);
  days += date.day - 1;
  return days * 86400UL + date.hour * 3600UL + date.minute * 60UL;
}

inline void settingsAdjustDateTime(SettingsDateTime &date, int field, int direction) {
  if (field == 0) {
    int days = settingsDaysInMonth(date.year, date.month);
    date.day = (date.day - 1 + direction + days) % days + 1;
  } else if (field == 1) {
    date.month = (date.month - 1 + direction + 12) % 12 + 1;
  } else if (field == 2) {
    date.year += direction;
    if (date.year < 2025) date.year = 2025;
    if (date.year > 2099) date.year = 2099;
  } else if (field == 3) {
    date.hour = (date.hour + direction + 24) % 24;
  } else if (field == 4) {
    date.minute = (date.minute + direction + 60) % 60;
  }
  int days = settingsDaysInMonth(date.year, date.month);
  if (date.day > days) date.day = days;
}
