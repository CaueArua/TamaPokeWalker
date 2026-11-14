#pragma once

#include <stdint.h>

/** Tests whether a nonzero millis() deadline is still in the future. */
inline bool deadlineActive(uint32_t now, uint32_t deadline) {
  return deadline != 0 && (int32_t)(now - deadline) < 0;
}

/** Tests whether a nonzero millis() deadline has elapsed. */
inline bool deadlineReached(uint32_t now, uint32_t deadline) {
  return deadline != 0 && (int32_t)(now - deadline) >= 0;
}

/** Returns the time remaining before a deadline, or zero if it is inactive. */
inline uint32_t deadlineRemaining(uint32_t now, uint32_t deadline) {
  return deadlineActive(now, deadline) ? deadline - now : 0;
}
