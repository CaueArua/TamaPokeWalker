#include "screen.h"
#include "touch_controller.h"

extern void markUiDirty();

Screen* activeScreen = nullptr;

void setScreen(Screen* next) {
  if (activeScreen) {
    activeScreen-> onExit();
  } 

  activeScreen = next;
  lockTouchBrief();
  
  if (activeScreen) {
    activeScreen->markDirty();
    activeScreen->onEnter();
  } else {
    markUiDirty(); // Return to the main pet screen.
  }
}
