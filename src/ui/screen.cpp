#include "screen.h"

extern void markUiDirty();
extern void lockTouchBrief(uint16_t ms = 160);

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
    markUiDirty(); // Volta para a tela principal do Pokémon
  }
}
