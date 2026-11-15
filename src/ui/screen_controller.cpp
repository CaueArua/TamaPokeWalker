#include "screen_controller.h"

#include <Arduino.h>

#include "../../pet.h"
#include "../../time_utils.h"
#include "battle_screen.h"
#include "clock_screen.h"
#include "dex_screen.h"
#include "activity_menu_screen.h"
#include "help_screen.h"
#include "keyboard_screen.h"
#include "menu_screen.h"
#include "main_screen.h"
#include "party_screen.h"
#include "pet_card_expedition_screen.h"
#include "power_manager.h"
#include "starter_screen.h"
#include "strength_screen.h"

extern bool cardDirty;
extern bool clockDirty;
extern bool helpDirty;
extern bool keyboardDirty;
extern bool gameMenuDirty;
extern bool gameMenuOpen;
extern uint32_t wildPromptUntil;
extern uint32_t perfRenderSkipCount;
extern Pet pet;
extern bool watermarkNeedsRedraw();

/** Returns the diagnostic identifier for the active screen. */
const char *screenName() {
  if (!activeScreen) {
    if (gameMenuOpen) return "game-menu";
    return screenOff ? "screen-off" : "main";
  }
  if (activeScreen == &dexScreen) return dexScreen.isDetail() ? "gallery-detail" : "gallery-grid";
  if (activeScreen->isMinigame()) return "game";
  if (activeScreen == &strengthTrainingScreen) return "sack";
  if (activeScreen == &battleScreen) return battleScreen.isResolved() ? "battle-result" : "battle";
  if (activeScreen == &keyboardScreen) return "keyboard";
  if (activeScreen == &helpScreen) return "help";
  if (activeScreen == &menuScreen) return "menu";
  if (activeScreen == &activityMenuScreen) return "activities";
  if (activeScreen == &petCardExpeditionScreen) return "expedition";
  if (activeScreen == &clockSettingsScreen) return "settings";
  if (activeScreen->isPetCardPage()) return "card";
  if (activeScreen == &partyScreen || activeScreen == &partyReplaceScreen) return "party";
  if (activeScreen == &starterScreen) return "starter";
  if (gameMenuOpen) return "game-menu";
  return "screen";
}

/** Returns whether the current view can skip rendering because it is clean. */
static bool staticScreenClean() {
  if (watermarkNeedsRedraw()) return false;
  if (activeScreen) return activeScreen->isStaticClean();
  if (gameMenuOpen) return !gameMenuDirty;
  return false;
}

/** Selects the render cadence for the active screen and power state. */
uint16_t renderIntervalMs() {
  if (screenOff) return 5000;
  if (activeScreen == &battleScreen) return battleScreen.isResolved() ? 320 : 190;
  if (activeScreen && (activeScreen->isMinigame() || activeScreen == &strengthTrainingScreen)) return 115;
  if (activeScreen == &dexScreen ||
      (activeScreen && activeScreen->isPetCardPage()) ||
      activeScreen == &keyboardScreen || activeScreen == &clockSettingsScreen ||
      activeScreen == &helpScreen || activeScreen == &activityMenuScreen ||
      activeScreen == &petCardExpeditionScreen || gameMenuOpen) return powerSave ? 650 : 320;
  if (!powerSave) return 100;
  if (dimStage >= 2) return 650;
  if (dimStage >= 1) return 350;
  return 180;
}

/** Renders the active screen, or the main screen when no modular screen is active. */
void render() {
  if (watermarkNeedsRedraw()) {
    cardDirty = true;
    clockDirty = true;
    helpDirty = true;
    keyboardDirty = true;
    gameMenuDirty = true;
    battleScreen.markDirty();
    if (activeScreen) activeScreen->markDirty();
  }
  if (staticScreenClean()) {
    perfRenderSkipCount++;
    return;
  }
  if (activeScreen) {
    Screen *renderingScreen = activeScreen;
    renderingScreen->render();
    if (activeScreen == renderingScreen && renderingScreen->shouldClose())
      setScreen(nullptr);
    return;
  }
  mainScreen.render();
}

/** Routes vertical swipes to the active screen or the home screen. */
void onSwipeV(int direction) {
  if (pet.awaitingStarter()) return;
  if (wildPromptUntil && deadlineActive(millis(), wildPromptUntil)) return;
  if (wildPromptUntil) wildPromptUntil = 0;
  if (gameMenuOpen || pet.ceremony) return;
  if (activeScreen) {
    activeScreen->handleSwipeV(direction);
    return;
  }
  mainScreen.handleSwipeV(direction);
}

/** Routes horizontal swipes to the active screen or the home screen. */
void onSwipe(int direction) {
  if (pet.awaitingStarter()) return;
  if (wildPromptUntil && deadlineActive(millis(), wildPromptUntil)) return;
  if (wildPromptUntil) wildPromptUntil = 0;
  if (gameMenuOpen) return;
  if (activeScreen) {
    activeScreen->handleSwipe(direction);
    return;
  }
  mainScreen.handleSwipe(direction);
}

/** Routes taps to the active screen or the home screen. */
void onTap(int16_t x, int16_t y) {
  if (activeScreen) {
    Screen *tappedScreen = activeScreen;
    activeScreen->handleTap(x, y);
    if (activeScreen == tappedScreen && tappedScreen->shouldClose())
      setScreen(nullptr);
    return;
  }
  mainScreen.handleTap(x, y);
}
