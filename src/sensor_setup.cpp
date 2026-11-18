#include "startup.h"

#include "../imu.h"
#include "core/clock_service.h"
#include "hardware/power.h"
#include "ui/touch_controller.h"

/** Initializes the touch, clock, battery, and motion sensors. */
void initializeSensors() {
  touchBegin();
  Clock::begin();
  batBegin();
  imuBegin();
}
