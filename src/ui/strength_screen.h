#pragma once
#include "screen.h"
#include "../../pet.h"

class StrengthTrainingScreen : public Screen {
private:
  uint32_t sackUntil = 0;
  uint32_t sackOverUntil = 0;
  uint16_t sackHits = 0;
  float sackShake = 0;
  uint8_t sackGain = 0;
  bool sackNewHi = false;

public:
  bool isOpen() const;
  void open();
  void handlePress(int16_t y);

  void onEnter() override;
  bool isStaticClean() const override { return false; }
  void render() override;
};

extern StrengthTrainingScreen strengthTrainingScreen;
