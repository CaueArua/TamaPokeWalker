#pragma once
#include "screen.h"

class BattleScreen : public Screen {
public:
  bool isStaticClean() const override;
  void render() override;
  void handleTap(int16_t x, int16_t y) override;
};

extern BattleScreen battleScreen;
