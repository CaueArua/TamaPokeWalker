// Host-side checks for src/core/clock.h. Build with a C++17 compiler from the repository root:
//   g++ -std=c++17 tests/clock_test.cpp -o clock_test
#include <cstdio>
#include "../src/core/clock.h"

static int failures = 0;

static void check(const char *name, long long got, long long want) {
  if (got != want) {
    std::printf("FAIL %s: got %lld, want %lld\n", name, got, want);
    ++failures;
  }
}

int main() {
  check("1900 is not leap", clockLeapYear(1900), 0);
  check("2000 is leap", clockLeapYear(2000), 1);
  check("2024 is leap", clockLeapYear(2024), 1);
  check("2100 is not leap", clockLeapYear(2100), 0);
  check("feb 2024 has 29 days", clockDaysInMonth(2024, 2), 29);
  check("feb 2025 has 28 days", clockDaysInMonth(2025, 2), 28);

  // 2000-02-29 00:00 UTC.
  ClockDateTime leapDay = clockDateTime(951782400UL);
  check("leap day year", leapDay.year, 2000);
  check("leap day month", leapDay.month, 2);
  check("leap day day", leapDay.day, 29);
  check("leap day hour", leapDay.hour, 0);

  // 2024-12-31 23:59:30 UTC, half a minute before the 2025 epoch.
  ClockDateTime yearEnd = clockDateTime(1735689570UL);
  check("year end year", yearEnd.year, 2024);
  check("year end month", yearEnd.month, 12);
  check("year end day", yearEnd.day, 31);
  check("year end hour", yearEnd.hour, 23);
  check("year end minute", yearEnd.minute, 59);
  check("year end second", yearEnd.second, 30);
  check("round trip year end", epochFromDateTime(yearEnd), 1735689570UL);
  check("round trip leap day", epochFromDateTime(leapDay), 951782400UL);

  // Day wraps forward past the end of January.
  ClockDateTime jan31{2025, 1, 31, 12, 0, 0};
  clockAdjustDateTime(jan31, 0, 1);
  check("day wraps to 1", jan31.day, 1);
  // Changing month to February clamps the day to 28.
  ClockDateTime day31{2025, 1, 31, 12, 0, 0};
  clockAdjustDateTime(day31, 1, 1);
  check("month change clamps day", day31.day, 28);
  // Year is clamped to the screen range.
  ClockDateTime low{2025, 6, 10, 0, 0, 0};
  clockAdjustDateTime(low, 2, -1);
  check("year floor", low.year, 2025);
  ClockDateTime high{2099, 6, 10, 0, 0, 0};
  clockAdjustDateTime(high, 2, 1);
  check("year ceiling", high.year, 2099);

  check("backward clock yields zero", clockElapsedMinutes(1000UL, 2000UL), 0);
  check("partial minute floors", clockElapsedMinutes(1000UL + 119UL, 1000UL), 1);
  check("two minutes", clockElapsedMinutes(1000UL + 120UL, 1000UL), 2);
  check("catch-up capped at 14 days", clockElapsedMinutes(1000000000UL, 0UL), CLOCK_MAX_OFFLINE_MINUTES);

  if (failures == 0) std::printf("All clock tests passed.\n");
  return failures == 0 ? 0 : 1;
}
