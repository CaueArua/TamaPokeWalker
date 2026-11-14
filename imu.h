#pragma once

#include <Arduino.h>

/** Initializes the QMI8658 on the shared I2C bus. */
bool imuBegin();
/** Returns whether the IMU was detected and initialized. */
bool imuOk();
/** Polls the sensor when the requested interval has elapsed. */
void imuPoll(uint32_t nowMs, uint16_t intervalMs);
/** Consumes and returns a pending shake edge event. */
bool imuShakeEdge();
/** Returns and clears the number of steps accumulated since the last call. */
uint16_t imuTakeSteps();
/** Returns the latest acceleration magnitude in g, or zero if unavailable. */
float imuLastMagG();
/** Returns the latest gyroscope magnitude in degrees per second. */
float imuLastGyroDps();
/** Returns the detected I2C address, or zero if no IMU was found. */
uint8_t imuAddr();
/** Returns the raw hardware pedometer count. */
uint32_t imuPedometer();
/** Returns the software step count derived from acceleration samples. */
uint32_t imuSoftwarePedometer();
