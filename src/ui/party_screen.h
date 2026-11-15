#pragma once
#include "screen.h"
#include "../../pet.h"
#include "../../sdmon.h"

class PartyScreen : public Screen {
private:
  Screen *returnScreen = nullptr;
  PmdMon slotPmd[PARTY_SIZE];
  /** Returns to the screen that opened the party. */
  void close();

public:
  /** Opens the party and records the screen to return to. */
  void open(Screen *returnTo);

  /** Loads party sprite data when the party screen becomes active. */
  void onEnter() override;
  /** Releases party sprite data when the screen closes. */
  void onExit() override;
  /** Party sprites animate, so the screen must be rendered continuously. */
  bool isStaticClean() const override { return false; }

  /** Draws the current party slots. */
  void render() override;
  /** Handles party selection and slot actions. */
  void handleTap(int16_t x, int16_t y) override;

};

class PartyReplaceScreen : public Screen {
private:
  int16_t pendingDex = 0;
  uint8_t pendingLevel = 1;
  bool pendingShiny = false;
  Screen *returnScreen = nullptr;
  PmdMon slotPmd[PARTY_SIZE];
  PmdMon candidatePmd;
  /** Returns to the screen that offered the replacement choice. */
  void closeReplacement();

public:
  /** Sets the caught species offered for a party replacement. */
  void setCandidate(int16_t dex, uint8_t level, bool shiny) {
    pendingDex = dex;
    pendingLevel = level;
    pendingShiny = shiny;
    returnScreen = activeScreen;
  }

  /** Loads party and candidate sprite data when replacement opens. */
  void onEnter() override;
  /** Releases party and candidate sprite data when replacement closes. */
  void onExit() override;
  /** Replacement sprites animate, so the screen must be rendered continuously. */
  bool isStaticClean() const override { return false; }

  /** Draws the party slots and replacement candidate. */
  void render() override;
  /** Handles selection of the party slot to replace. */
  void handleTap(int16_t x, int16_t y) override;
};

extern PartyScreen partyScreen;
extern PartyReplaceScreen partyReplaceScreen;
