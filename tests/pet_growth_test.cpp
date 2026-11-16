// Host-side check for pet_growth.h. Build with a C++17 compiler from the repository root:
//   g++ -std=c++17 tests/pet_growth_test.cpp -o pet_growth_test
#include <cstdio>
#include "../pet_growth.h"

static int failures = 0;

static void check(const char *name, long long got, long long want) {
  if (got != want) {
    std::printf("FAIL %s: got %lld, want %lld\n", name, got, want);
    ++failures;
  }
}

int main() {
  // Level 1 and below have no accumulated experience for any group.
  for (int g = 0; g <= 5; ++g) {
    check("level 1 is zero", getTotalExp(static_cast<GrowthRate>(g), 1), 0);
  }

  check("fast L10", getTotalExp(GrowthRate::Fast, 10), 800);
  check("fast L100", getTotalExp(GrowthRate::Fast, 100), 800000);

  check("medium fast L10", getTotalExp(GrowthRate::MediumFast, 10), 1000);
  check("medium fast L100", getTotalExp(GrowthRate::MediumFast, 100), 1000000);

  check("medium slow L2", getTotalExp(GrowthRate::MediumSlow, 2), 9);
  check("medium slow L10", getTotalExp(GrowthRate::MediumSlow, 10), 560);
  check("medium slow L100", getTotalExp(GrowthRate::MediumSlow, 100), 1059860);

  check("slow L10", getTotalExp(GrowthRate::Slow, 10), 1250);
  check("slow L100", getTotalExp(GrowthRate::Slow, 100), 1250000);

  check("erratic L2", getTotalExp(GrowthRate::Erratic, 2), 15);
  check("erratic L60", getTotalExp(GrowthRate::Erratic, 60), 194400);
  check("erratic L70", getTotalExp(GrowthRate::Erratic, 70), 276458);
  check("erratic L100", getTotalExp(GrowthRate::Erratic, 100), 600000);

  check("fluctuating L10", getTotalExp(GrowthRate::Fluctuating, 10), 540);
  check("fluctuating L20", getTotalExp(GrowthRate::Fluctuating, 20), 5440);
  check("fluctuating L40", getTotalExp(GrowthRate::Fluctuating, 40), 66560);
  check("fluctuating L100", getTotalExp(GrowthRate::Fluctuating, 100), 1640000);

  // Levels above 100 cap at the level-100 total.
  check("cap above 100", getTotalExp(GrowthRate::Slow, 150), getTotalExp(GrowthRate::Slow, 100));

  // Remaining experience to evolve, derived from the current level.
  check("evolve fast 10->16", getExpToEvolve(GrowthRate::Fast, 10, 16), 3276 - 800);
  // Explicit current experience overrides the level-derived value.
  check("evolve explicit exp", getExpToEvolve(GrowthRate::Fast, 10, 16, 1000), 3276 - 1000);
  // Evolution level above 100 is clamped to 100.
  check("evolve clamp target", getExpToEvolve(GrowthRate::Fast, 10, 150), 800000 - 800);
  // Already at or past the evolution level.
  check("evolve already there", getExpToEvolve(GrowthRate::Fast, 16, 16), 0);
  // Experience at the cap cannot grow, so nothing remains.
  check("evolve at cap", getExpToEvolve(GrowthRate::Fast, 50, 60, 800000), 0);
  // Level-100 pets never evolve by experience.
  check("evolve level 100", getExpToEvolve(GrowthRate::Fast, 100, 110), 0);

  if (failures == 0) std::printf("All pet_growth tests passed.\n");
  return failures == 0 ? 0 : 1;
}
