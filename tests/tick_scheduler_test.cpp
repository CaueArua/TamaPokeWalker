// Host-side checks for src/core/tick_scheduler.h. Build with a C++17 compiler from the repository root:
//   g++ -std=c++17 tests/tick_scheduler_test.cpp -o tick_scheduler_test
#include <cstdio>
#include "../src/core/tick_scheduler.h"

static int failures = 0;

static void check(const char *name, long long got, long long want) {
  if (got != want) {
    std::printf("FAIL %s: got %lld, want %lld\n", name, got, want);
    ++failures;
  }
}

int main() {
  TickScheduler s{60000UL};
  s.reset(1000);
  check("no tick before period", s.consume(60999), 0);
  check("one tick at period", s.consume(61000), 1);
  check("no drift after late poll", s.consume(121000), 1);
  check("late poll catches up", s.consume(301000), 3);
  check("partial remainder kept", s.consume(360999), 0);
  check("reset restarts schedule", (s.reset(0), s.consume(59999)), 0);

  return failures == 0 ? (std::printf("All tick_scheduler tests passed.\n"), 0) : 1;
}
