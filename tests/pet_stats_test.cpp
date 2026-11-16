// Host-side check for calcularStat in pet_stats.h. Build with a C++17 compiler from the repository root:
//   g++ -std=c++17 tests/pet_stats_test.cpp -o pet_stats_test
#include <cstdio>
#include "../pet_stats.h"

static int failures = 0;

static void check(const char *name, long long got, long long want) {
  if (got != want) {
    std::printf("FAIL %s: got %lld, want %lld\n", name, got, want);
    ++failures;
  }
}

int main() {
  check("neutral L50", calcularStat(80, 10, 0, 50), 90);
  // EV contribution is ev / 4 with integer division.
  check("ev 3 adds nothing", calcularStat(100, 0, 3, 100), 205);
  check("ev 4 adds one", calcularStat(100, 0, 4, 100), 206);
  check("level 1 minimum", calcularStat(50, 0, 0, 1), 6);
  check("max inputs neutral", calcularStat(255, 31, 252, 100), 609);
  // Blaziken Atk at L99 is 279 neutral; nature multipliers floor the result.
  check("positive nature", calcularStat(120, 22, 63, 99, 1.1), 306);
  check("negative nature", calcularStat(120, 22, 63, 99, 0.9), 251);
  check("explicit neutral", calcularStat(120, 22, 63, 99, 1.0), 279);
  check("explicit neutral SpA", calcularStat(110, 28, 87, 99, 1.0), 271);

  if (failures == 0) std::printf("All pet_stats tests passed.\n");
  return failures == 0 ? 0 : 1;
}
