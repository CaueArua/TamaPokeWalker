#include "startup.h"

#include "../imu.h"
#include "../rtcbat.h"
#include "ui/touch_controller.h"

/** Initializes the touch, clock, battery, and motion sensors. */
void initializeSensors() {
  touchBegin();
  rtcBegin();
  batBegin();
  imuBegin();
}
