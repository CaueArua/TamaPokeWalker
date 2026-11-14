#include "imu.h"
#include "pin_config.h"

#include <math.h>
#include <Wire.h>

#include "time_utils.h"

#define QMI8658_ADDR_LOW  0x6B
#define QMI8658_ADDR_HIGH 0x6A

static bool ok = false;
static uint8_t foundAddr = 0;
static uint32_t lastPoll = 0;
static uint32_t bootIgnoreUntil = 0;
static uint32_t shakeIgnoreUntil = 0;
static bool shakePending = false;
static float lastMag = 0;
static float lastGyro = 0;
static uint32_t lastPed = 0;
static bool pedInited = false;
static uint16_t stepBank = 0;
static uint32_t softPed = 0;
static float walkGravity = 0;
static bool walkPeak = false;
static uint32_t walkLastPeak = 0;
static uint8_t walkCadence = 0;

bool imuOk() {
  return ok;
}

float imuLastMagG() {
  return lastMag;
}

float imuLastGyroDps() {
  return lastGyro;
}

uint8_t imuAddr() {
  return foundAddr;
}

uint32_t imuPedometer() {
  return lastPed;
}

uint32_t imuSoftwarePedometer() {
  return softPed;
}

/** Adds detected steps to the pending batch and software pedometer total. */
static void bankSteps(uint16_t count) {
  uint32_t next = (uint32_t)stepBank + count;
  stepBank = next > 60000UL ? 60000 : (uint16_t)next;
  softPed += count;
}

/** Detects walking steps from acceleration magnitude and cadence. */
static void detectSoftwareStep(uint32_t nowMs, float mag) {
  if (walkGravity <= 0.01f) {
    walkGravity = mag;
    return;
  }

  float signal = mag - walkGravity;
  float motion = fabsf(signal);
  float alpha = motion < 0.08f ? 0.12f : 0.025f;
  walkGravity += (mag - walkGravity) * alpha;
  if (deadlineActive(nowMs, bootIgnoreUntil)) return;

  if (walkPeak) {
    if (signal <= 0.025f) walkPeak = false;
    return;
  }
  if (signal < 0.085f) return;
  walkPeak = true;

  uint32_t gap = walkLastPeak ? nowMs - walkLastPeak : 0;
  if (walkLastPeak && gap < 280UL) return;
  if (!walkLastPeak || gap > 1400UL) {
    walkLastPeak = nowMs;
    walkCadence = 1;
    return;
  }

  walkLastPeak = nowMs;
  if (walkCadence == 1) {
    bankSteps(2);
    walkCadence = 2;
  } else {
    bankSteps(1);
    if (walkCadence < 255) walkCadence++;
  }
}

/** Writes a register on the detected QMI8658 device. */
static void writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

/** Reads a register from the detected QMI8658 device. */
static uint8_t readReg(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return 0;
  return Wire.read();
}

/** Verifies the sensor identity, configures its ranges, and starts sampling. */
static bool probeAndStart(uint8_t addr) {
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() != 0) return false;

  uint8_t chipId = readReg(addr, 0x00); // WHO_AM_I must return 0x05.
  if (chipId != 0x05) return false;

  // Software reset (0x60 = 0xB0).
  writeReg(addr, 0x60, 0xB0);
  delay(15);

  // CTRL1 (0x02): enable I2C register auto-increment (0x40).
  writeReg(addr, 0x02, 0x40);

  // CTRL2 (0x03): set the accelerometer to 2 g and 62.5 Hz ODR (0x05).
  writeReg(addr, 0x03, 0x05);

  // CTRL3 (0x04): set the gyroscope to 512 dps and 112.1 Hz ODR (0x54).
  writeReg(addr, 0x04, 0x54);

  // CTRL7 (0x08): enable the accelerometer (bit 0) and gyroscope (bit 1).
  writeReg(addr, 0x08, 0x03);

  delay(40);
  foundAddr = addr;
  return true;
}

/** Reads and converts all six acceleration and gyroscope axes. */
static bool readImuDirect(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  if (foundAddr == 0) return false;

  // Read 12 consecutive bytes starting at register 0x35 (AX_L through GZ_H).
  Wire.beginTransmission(foundAddr);
  Wire.write(0x35);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(foundAddr, (uint8_t)12) != 12) return false;

  int16_t rawAx = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawAy = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawAz = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawGx = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawGy = (int16_t)(Wire.read() | (Wire.read() << 8));
  int16_t rawGz = (int16_t)(Wire.read() | (Wire.read() << 8));

  // A 2 g accelerometer range uses 16384 LSB/g.
  ax = rawAx / 16384.0f;
  ay = rawAy / 16384.0f;
  az = rawAz / 16384.0f;

  // A 512 dps gyroscope range uses 64 LSB/dps.
  gx = rawGx / 64.0f;
  gy = rawGy / 64.0f;
  gz = rawGz / 64.0f;

  return true;
}

bool imuBegin() {
  ok = false;
  foundAddr = 0;
  lastMag = 0;
  lastGyro = 0;
  lastPed = 0;
  pedInited = false;
  stepBank = 0;
  softPed = 0;
  walkGravity = 0;
  walkPeak = false;
  walkLastPeak = 0;
  walkCadence = 0;
  shakePending = false;
  lastPoll = 0;

  if (!probeAndStart(QMI8658_ADDR_LOW) &&
      !probeAndStart(QMI8658_ADDR_HIGH)) {
    Serial.println("QMI8658 no detectado");
    return false;
  }

  bootIgnoreUntil = millis() + 400;
  ok = true;
  Serial.printf("QMI8658 ok (I2C direto) addr=0x%02X\n", foundAddr);
  return true;
}

void imuPoll(uint32_t nowMs, uint16_t intervalMs) {
  if (!ok) return;
  if (intervalMs < 25) intervalMs = 25;
  if (lastPoll && (uint32_t)(nowMs - lastPoll) < intervalMs) return;
  lastPoll = nowMs ? nowMs : 1;

  float x = 0, y = 0, z = 0;
  float gx = 0, gy = 0, gz = 0;
  if (readImuDirect(x, y, z, gx, gy, gz)) {
    lastMag = sqrtf(x * x + y * y + z * z);
    detectSoftwareStep(nowMs, lastMag);
    lastGyro = sqrtf(gx * gx + gy * gy + gz * gz);
    lastPed = softPed;
  }

  bool hot = lastMag >= 1.45f || lastGyro >= 80.0f;
  if (deadlineActive(nowMs, bootIgnoreUntil)) {
    return;
  }
  if (hot && !deadlineActive(nowMs, shakeIgnoreUntil)) {
    shakePending = true;
    shakeIgnoreUntil = nowMs + 280;
  }
}

bool imuShakeEdge() {
  bool hit = shakePending;
  shakePending = false;
  return hit;
}

uint16_t imuTakeSteps() {
  uint16_t n = stepBank;
  stepBank = 0;
  return n;
}
