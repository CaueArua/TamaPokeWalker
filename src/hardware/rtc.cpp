#include "rtc.h"
#include "../../pin_config.h"
#include <Wire.h>
#include <SensorPCF85063.hpp>

static SensorPCF85063 rtc;
static bool rtcOk = false;

bool rtcBegin() {
  rtcOk = rtc.begin(Wire, IIC_SDA, IIC_SCL);
  if (!rtcOk) Serial.println("PCF85063 no detectado");
  return rtcOk;
}

bool rtcIsPresent() { return rtcOk; }

bool rtcReadDateTime(ClockDateTime &out) {
  if (!rtcOk) return false;
  RTC_DateTime t = rtc.getDateTime();
  out = { t.getYear(), t.getMonth(), t.getDay(), t.getHour(), t.getMinute(), t.getSecond() };
  return true;
}

bool rtcWriteDateTime(const ClockDateTime &in) {
  if (!rtcOk) return false;
  rtc.setDateTime(RTC_DateTime(in.year, in.month, in.day, in.hour, in.minute, in.second));
  return true;
}
