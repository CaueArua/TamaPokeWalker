#include "startup.h"

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>

#include "../i18n.h"
#include "../pin_config.h"
#include "../rtcbat.h"

extern Arduino_CO5300 *panel;
extern Arduino_Canvas *gfx;
const char *firmwareVersion();

/** Initializes serial, the I2C bus, and the display hardware. */
void initializeHardware() {
  Serial.setRxBufferSize(8192);  // SD transfers arrive in 2 KB blocks.
  Serial.begin(115200);
  // Critical: without this, Serial.print can block gameplay when no host
  // monitor is connected and the USB CDC TX buffer fills. A zero timeout
  // drops those optional log messages instead of blocking the firmware.
  Serial.setTxTimeoutMs(0);
  Serial.printf("TamaPoke fw v%s\n", firmwareVersion());
  loadLang();  // Load the saved language (Spanish by default).
  Wire.begin(IIC_SDA, IIC_SCL);
  // The CST9217 touch controller, AXP2101 PMU, and PCF85063 RTC share I2C.
  // This timeout protects PMU/RTC operations. SensorLib does not honor it for
  // touch reads; the touch controller gates reads on INT.
  Wire.setTimeOut(50);

  // Critical: enable the panel power rail (BLDO1 = OLED VDD 3.3 V) before
  // initializing the display. A PMU reset can leave this rail off and the
  // display black even while the rest of the board is running.
  pmuEnablePanel();

  // Use 80 MHz QSPI instead of the 40 MHz default; framebuffer flush is the
  // frame-rate bottleneck (~56 ms at 40 MHz). Reduce this if the panel corrupts.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");
  panel->setBrightness(180);
}
