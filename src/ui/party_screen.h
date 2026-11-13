#pragma once
#include "screen.h"
#include "../../pet.h"
#include "../../sdmon.h"

class PartyScreen : public Screen {
private:
  PmdMon slotPmd[PARTY_SIZE];

public:
  void onEnter() override;
  void onExit() override;
  bool isStaticClean() const override { return false; }

  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  void handleSwipe(int dir) override;
};

class PartyReplaceScreen : public Screen {
private:
  int16_t pendingDex = 0;
  uint8_t pendingLevel = 1;
  bool pendingShiny = false;
  PmdMon slotPmd[PARTY_SIZE];
  PmdMon candidatePmd;

public:
  void setCandidate(int16_t dex, uint8_t level, bool shiny) {
    pendingDex = dex;
    pendingLevel = level;
    pendingShiny = shiny;
  }

  void onEnter() override;
  void onExit() override;
  bool isStaticClean() const override { return false; }

  void render() override;
  void handleTap(int16_t x, int16_t y) override;
};

extern PartyScreen partyScreen;
extern PartyReplaceScreen partyReplaceScreen;
