#pragma once

#include <stdint.h>

// Pure calendar and elapsed-time helpers. All values are UTC.
// No hardware access here; see clock.cpp for the RTC-backed service.

#define CLOCK_MAX_OFFLINE_MINUTES (14UL * 24UL * 60UL)

struct ClockDateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

inline bool clockLeapYear(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline int clockDaysInMonth(int year, int month) {
  static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return days[month - 1] + (month == 2 && clockLeapYear(year) ? 1 : 0);
}

inline ClockDateTime clockDateTime(uint32_t epoch) {
  uint32_t days = epoch / 86400UL;
  int year = 1970;
  while (days >= (uint32_t)(clockLeapYear(year) ? 366 : 365)) {
    days -= clockLeapYear(year) ? 366 : 365;
    ++year;
  }
  int month = 1;
  while (days >= (uint32_t)clockDaysInMonth(year, month)) {
    days -= clockDaysInMonth(year, month);
    ++month;
  }
  return { year, month, (int)days + 1, (int)((epoch / 3600UL) % 24),
           (int)((epoch / 60UL) % 60), (int)(epoch % 60UL) };
}

inline uint32_t epochFromDateTime(const ClockDateTime &date) {
  uint32_t days = 0;
  for (int year = 1970; year < date.year; ++year) days += clockLeapYear(year) ? 366 : 365;
  for (int month = 1; month < date.month; ++month) days += clockDaysInMonth(date.year, month);
  days += date.day - 1;
  return days * 86400UL + date.hour * 3600UL + date.minute * 60UL + date.second;
}

// Whole minutes between two epochs, capped to the offline catch-up limit.
// A clock that moved backwards yields zero.
inline uint32_t clockElapsedMinutes(uint32_t nowEpoch, uint32_t seenEpoch) {
  if (nowEpoch <= seenEpoch) return 0;
  uint32_t mins = (nowEpoch - seenEpoch) / 60UL;
  return mins > CLOCK_MAX_OFFLINE_MINUTES ? CLOCK_MAX_OFFLINE_MINUTES : mins;
}

// Steps one field of a date by direction (+1 or -1), wrapping day, month,
// hour and minute. Year is clamped to the screen range. Day is clamped to
// the length of the resulting month.
inline void clockAdjustDateTime(ClockDateTime &date, int field, int direction) {
  if (field == 0) {
    int days = clockDaysInMonth(date.year, date.month);
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
  int days = clockDaysInMonth(date.year, date.month);
  if (date.day > days) date.day = days;
}
