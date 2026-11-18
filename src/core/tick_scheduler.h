#pragma once

#include <stdint.h>

// Counts whole fixed-period ticks. The schedule advances by the period, not to
// "now", so ticks do not drift when the caller polls late.
struct TickScheduler {
  uint32_t periodMs;
  uint32_t lastMs = 0;

  /** Restarts the schedule so the next tick is one period after nowMs. */
  void reset(uint32_t nowMs) { lastMs = nowMs; }

  /** Returns how many whole periods have elapsed and advances the schedule by them. */
  uint32_t consume(uint32_t nowMs) {
    uint32_t count = 0;
    while (nowMs - lastMs >= periodMs) {
      lastMs += periodMs;
      count++;
    }
    return count;
  }
};
