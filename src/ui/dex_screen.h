#pragma once
#include "screen.h"
#include "../../sdmon.h"

class DexScreen : public Screen {
private:
  PmdMon galleryPmd;
  bool dirty = true;
  int page = 0;
  int16_t detail = 0;
  uint8_t filter = 0;
  uint8_t region = 0;
  Screen *returnScreen = nullptr;
  bool detailReturnsToScreen = false;

  /** Returns whether a species passes the current Dex filters. */
  bool dexVisible(int16_t dex) const;
  /** Counts species visible under the current filters. */
  uint16_t filteredCount() const;
  /** Returns the number of pages needed for the filtered list. */
  int pageCount() const;
  /** Returns the species at an index in the filtered list. */
  int16_t dexAt(uint16_t index) const;
  /** Opens the detail view for a species. */
  void showDetail(int16_t dex);
  /** Returns to the screen that opened the Dex. */
  void closeDex();

public:
  /** Opens the filtered Dex grid at its current page. */
  void openGrid(Screen *returnTo = nullptr);
  /** Opens the detail view for the specified species. */
  void openDetail(int16_t dex, Screen *returnTo = nullptr);
  /** Returns whether a species detail view is displayed. */
  bool isDetail() const { return detail != 0; }
  /** Returns whether the Dex screen needs to be redrawn. */
  bool isDirty() const { return dirty; }

  /** Resets transient screen state when the Dex becomes active. */
  void onEnter() override;
  /** Releases sprite data when the Dex becomes inactive. */
  void onExit() override;
  /** Returns whether the current grid can skip rendering. */
  bool isStaticClean() const override { return activeScreen == this && !detail && !dirty; }
  /** Marks the Dex screen as needing a redraw. */
  void markDirty() override { dirty = true; }

  /** Draws the grid or selected species detail view. */
  void render() override;
  /** Handles taps on the Dex grid and detail controls. */
  void handleTap(int16_t x, int16_t y) override;
  /** Changes the Dex page or exits the detail view. */
  void handleSwipe(int dir) override;
};

extern DexScreen dexScreen;

/** Draws an encoded sprite thumbnail centered in its cell. */
void drawThumb(const uint8_t *b, int x, int y, int s, bool sil);
