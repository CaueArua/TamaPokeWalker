#include "power.h"
#include "../../pin_config.h"  // define XPOWERS_CHIP_AXP2101
#include <Wire.h>
#include <XPowersLib.h>

static XPowersPMU pmu;
static bool pmuOk = false;

/** Applies the configured USB current and battery charging limits. */
static void configureBatteryCharger() {
  if (!pmuOk) return;
  pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_900MA);
  pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_100MA);
  pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_100MA);
  pmu.enableChargerTerminationLimit();
  pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  pmu.enableCellbatteryCharge();
  Serial.printf("AXP2101 charger: vbusLim=%u chgCur=%u target=%u\n",
                pmu.getVbusCurrentLimit(), pmu.getChargerConstantCurr(),
                pmu.getChargeTargetVoltage());
}

bool batBegin() {
  pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!pmuOk) Serial.println("AXP2101 no detectado");
  else configureBatteryCharger();
  return pmuOk;
}

// Powers the AMOLED panel from the AXP2101 BLDO1 3.3 V rail. The rail may be
// disabled after a PMU reset, leaving the display black while the rest of the
// board still runs. Call this before gfx->begin().
void pmuEnablePanel() {
  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("AXP2101 no detectado (pmuEnablePanel)");
    return;
  }
  pmu.setBLDO1Voltage(3300);   // OLED VDD
  pmu.enableBLDO1();
}

// Cache power state for about two seconds to avoid per-frame I2C traffic and
// brightness flicker.
static uint32_t powerCacheT = 0;
static uint32_t powerCacheMs = 2000;
static int cachedPct = -1;
static bool cachedCharging = false, cachedUsb = true;

/** Refreshes cached battery and USB state when the cache expires. */
static void refreshPower() {
  uint32_t now = millis();
  if (powerCacheT && now - powerCacheT < powerCacheMs) return;
  powerCacheT = now ? now : 1;
  if (!pmuOk) { cachedPct = -1; cachedCharging = false; cachedUsb = true; return; }
  cachedPct = pmu.isBatteryConnect() ? pmu.getBatteryPercent() : -1;
  cachedCharging = pmu.isCharging();
  cachedUsb = pmu.isVbusIn();
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
bool usbPresent() { refreshPower(); return cachedUsb; }

void setPowerCacheInterval(uint32_t ms) {
  if (ms < 500) ms = 500;
  if (ms > 30000) ms = 30000;
  powerCacheMs = ms;
}

void pwrSetup() {
  if (!pmuOk) return;
  pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);
  pmu.clearIrqStatus();
}

bool pwrShortPressed() {
  if (!pmuOk) return false;
  pmu.getIrqStatus();
  bool hit = pmu.isPekeyShortPressIrq();
  if (hit) pmu.clearIrqStatus();
  return hit;
}
