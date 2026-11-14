#pragma once

#include <stdint.h>

extern const uint16_t BIOME_SOIL[6];
extern const uint16_t STARS[6][2];

struct PetBehavior {
  uint8_t mode = 0;
  uint8_t act = 0;
  uint32_t t0 = 0;
  uint32_t until = 0;
  float x = 233;
  float targetX = 233;
};

struct BathBubble {
  int16_t x, y;
  uint8_t r, ph;
};

struct MainButton {
  int16_t cx, cy;
  const char *const *icon;
};

struct GameMenuTile {
  int16_t x, y, w, h;
  uint8_t game;
};

class MainScreen {
public:
  /** Advances main-screen animations and timed pet events. */
  void update(uint32_t now);
  /** Updates walking rewards and refreshes the related card state. */
  void updateWalkCards(uint16_t walked, uint32_t stepsBefore, uint32_t stepDayBefore);
  /** Updates the expedition HUD and its timed state. */
  void updateExpedition(uint32_t now);
  /** Draws the pet, main-screen overlays, and home controls. */
  void render();
  /** Returns whether the pet can respond to a shake interaction. */
  bool readyForShake() const;
  /** Handles taps on the pet and main-screen controls. */
  void handleTap(int16_t x, int16_t y);
  /** Opens the related screen selected by a horizontal home swipe. */
  void handleSwipe(int dir);
  /** Opens the clock or pet card selected by a vertical home swipe. */
  void handleSwipeV(int dir);
  /** Schedules the next random wild encounter. */
  void scheduleNextWild(uint32_t now);
  /** Returns whether a point is inside the current pet-event hit area. */
  bool inPetEventHit(int16_t x, int16_t y) const;
  /** Accepts the currently displayed pet event. */
  void acceptPetEvent();
  /** Handles a tap on the pending wild-encounter prompt. */
  bool handleWildPromptTap(int16_t x, int16_t y);
  /** Handles a tap on the evolution or farewell choice dialog. */
  bool handleChoiceTap(int16_t x, int16_t y);
  /** Opens the pet-release confirmation prompt. */
  void beginReleaseConfirmation();
  /** Handles a response to the pet-release confirmation prompt. */
  bool handleReleaseTap(int16_t x, int16_t y);
  /** Starts the bath interaction and its animation. */
  void startBath();
};

extern PetBehavior beh;
extern BathBubble bubbles[14];
extern MainButton buttons[4];
extern const GameMenuTile GAME_MENU_TILES[5];
extern MainScreen mainScreen;
