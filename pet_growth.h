#pragma once
#include <algorithm>

/** Experience growth groups used by the main-series games. */
enum class GrowthRate { Fast, MediumFast, MediumSlow, Slow, Erratic, Fluctuating };

/** Total experience needed to reach a level (1..100). Levels at or below 1 return 0. */
inline long long getTotalExp(GrowthRate group, int level) {
  if (level <= 1) return 0;
  if (level > 100) level = 100;
  const long long L = level;
  switch (group) {
    case GrowthRate::Fast:
      return (4 * L * L * L) / 5;
    case GrowthRate::MediumFast:
      return L * L * L;
    case GrowthRate::MediumSlow:
      return ((6 * L * L * L) / 5) - (15 * L * L) + (100 * L) - 140;
    case GrowthRate::Slow:
      return (5 * L * L * L) / 4;
    case GrowthRate::Erratic:
      if (L < 50) return (L * L * L * (100 - L)) / 50;
      if (L < 68) return (L * L * L * (150 - L)) / 100;
      if (L < 98) return (L * L * L * ((1911 - 10 * L) / 3)) / 500;
      return (L * L * L * (160 - L)) / 100;
    case GrowthRate::Fluctuating:
      if (L < 15) return (L * L * L * (((L + 1) / 3) + 24)) / 50;
      if (L < 36) return (L * L * L * (L + 14)) / 50;
      return (L * L * L * ((L / 2) + 32)) / 50;
  }
  return 0;
}

/**
 * Experience still needed to reach evolutionLevel from currentLevel.
 * currentExp < 0 means "derive it from currentLevel". Returns 0 at the level cap.
 */
inline long long getExpToEvolve(GrowthRate group, int currentLevel, int evolutionLevel,
                                long long currentExp = -1) {
  if (currentLevel >= 100 || currentLevel >= evolutionLevel) return 0;
  if (evolutionLevel > 100) evolutionLevel = 100;
  const long long maxExp = getTotalExp(group, 100);
  if (currentExp < 0) currentExp = getTotalExp(group, currentLevel);
  if (currentExp >= maxExp) return 0;
  const long long targetExp = getTotalExp(group, evolutionLevel);
  return std::max(0LL, targetExp - currentExp);
}
