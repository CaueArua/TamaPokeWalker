#include "touch_controller.h"

#include <Arduino.h>
#include <Wire.h>
#include <TouchDrvCSTXXX.hpp>

#include "../../audio.h"
#include "../../i18n.h"
#include "../../pet.h"
#include "../../pin_config.h"
#include "../../species_chirp.h"
#include "../../time_utils.h"
#include "main_screen.h"
#include "power_manager.h"
#include "screen.h"
#include "screen_controller.h"
#include "strength_screen.h"

static TouchDrvCST92xx touch;
static bool wasPressed = false;
static volatile bool touchIrqPending = false;
static bool swallowGesture = false;
static uint32_t ignoreTouchUntil = 0;
static int16_t gestureStartX = 0;
static int16_t gestureStartY = 0;
static int16_t gestureLastX = 0;
static int16_t gestureLastY = 0;
static uint32_t gestureStartedAt = 0;
static bool holdFired = false;

extern Pet pet;
extern uint32_t confirmUntil;

/** Records the touch interrupt without performing I2C work in the ISR. */
static void IRAM_ATTR touchIsr() {
  touchIrqPending = true;
}

void touchBegin() {
  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {
    touchOk = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 no detectado");
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);
}

void lockTouchBrief(uint16_t durationMs) {
  uint32_t until = millis() + durationMs;
  if (until > ignoreTouchUntil) ignoreTouchUntil = until;
  wasPressed = false;
  swallowGesture = true;
}

bool inPetZone(int16_t x, int16_t y) {
  return x > 110 && x < 356 && y > 95 && y < 310;
}

bool touchInputBusy(uint32_t now) {
  return wasPressed || touchIrqPending || deadlineActive(now, ignoreTouchUntil);
}

void notifyTouchWake() {
  touchIrqPending = true;
}

void suppressTouchUntil(uint32_t deadline) {
  ignoreTouchUntil = deadline;
  swallowGesture = true;
  wasPressed = false;
}

void pollTouch() {
  static uint32_t lastPoll = 0;
  uint32_t now = millis();
  if (deadlineActive(now, ignoreTouchUntil)) {
    touchIrqPending = false;
    wasPressed = false;
    return;
  }
  if (now - lastPoll < (powerSave && screenOff ? 80UL : 20UL)) return;
  lastPoll = millis();
  if (!touchIrqPending && !wasPressed) return;
  touchIrqPending = false;
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

  if (screenOff) {
    if (pressed && !wasPressed) {
      gestureStartX = gestureLastX = x;
      gestureStartY = gestureLastY = y;
      gestureStartedAt = now;
      holdFired = false;
      wasPressed = true;
    } else if (pressed) {
      gestureLastX = x;
      gestureLastY = y;
      uint32_t heldMs = millis() - gestureStartedAt;
      if (!holdFired && !swallowGesture && activeScreen &&
          abs(gestureLastX - gestureStartX) < 30 &&
          abs(gestureLastY - gestureStartY) < 30 &&
          heldMs >= activeScreen->longPressDelayMs()) {
        activeScreen->handleLongPress(gestureStartX, gestureStartY, heldMs);
        holdFired = true;
        return;
      }
      if (!holdFired && !swallowGesture && !activeScreen && heldMs > 3000 &&
          abs(gestureLastX - gestureStartX) < 30 &&
          abs(gestureLastY - gestureStartY) < 30 &&
          inPetZone(gestureStartX, gestureStartY) &&
          !pet.isEgg() && !confirmUntil && !pet.ceremony) {
        mainScreen.beginReleaseConfirmation();
        holdFired = true;
      }
    } else {
      wasPressed = false;
      holdFired = false;
    }
    return;
  }

  if (activeScreen == &strengthTrainingScreen) {
    if (pressed && !wasPressed) activeScreen->handlePress(y);
    wasPressed = pressed;
    return;
  }

  if (pressed && !wasPressed) {
    gestureStartX = gestureLastX = x;
    gestureStartY = gestureLastY = y;
    gestureStartedAt = millis();
    holdFired = false;
    swallowGesture = (dimStage > 0) || screenOff;
    screenOff = false;
    lastInteract = millis();
    if (!swallowGesture && activeScreen && activeScreen->handlesTapOnPress()) {
      activeScreen->handleTap(x, y);
      swallowGesture = true;
    }
  } else if (pressed) {
    gestureLastX = x;
    gestureLastY = y;
    if (!holdFired && !swallowGesture && !activeScreen &&
        millis() - gestureStartedAt > 3000 &&
        abs(gestureLastX - gestureStartX) < 30 &&
        abs(gestureLastY - gestureStartY) < 30 &&
        inPetZone(gestureStartX, gestureStartY) &&
        !pet.isEgg() && !confirmUntil && !pet.ceremony) {
      mainScreen.beginReleaseConfirmation();
      holdFired = true;
    }
  } else if (wasPressed) {
    lastInteract = millis();
    int dx = gestureLastX - gestureStartX;
    int dy = gestureLastY - gestureStartY;
    uint32_t durationMs = millis() - gestureStartedAt;
    if (!holdFired && !swallowGesture) {
      if (abs(dx) > 80 && abs(dy) < 70 && durationMs < 800)
        onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && durationMs < 800)
        onSwipeV(dy > 0 ? 1 : -1);
      else if (durationMs < 1500 && abs(dx) < 40 && abs(dy) < 40)
        onTap(gestureStartX, gestureStartY);
    }
  }
  wasPressed = pressed;
}
