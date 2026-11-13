#pragma once
#include "screen.h"
#include "../../sdmon.h"

class DexScreen : public Screen {
private:
  PmdMon galleryPmd;
  bool opened = false;
  bool dirty = true;
  int page = 0;
  int16_t detail = 0;
  uint8_t filter = 0;
  uint8_t region = 0;

  bool dexVisible(int16_t dex) const;
  uint16_t filteredCount() const;
  int pageCount() const;
  int16_t dexAt(uint16_t index) const;
  void showDetail(int16_t dex);

public:
  void openGrid();
  void openDetail(int16_t dex);
  bool isOpen() const { return opened; }
  bool isDetail() const { return detail != 0; }
  bool isDirty() const { return dirty; }

  void onEnter() override;
  void onExit() override;
  bool isStaticClean() const override { return opened && !detail && !dirty; }
  void markDirty() override { dirty = true; }

  void render() override;
  void handleTap(int16_t x, int16_t y) override;
  void handleSwipe(int dir) override;
};

extern DexScreen dexScreen;

void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);
