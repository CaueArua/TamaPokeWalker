#pragma once
#include <Arduino.h>
#include <FS.h>

/**
 * Legacy TPK1 sprite format. PMD/TPK2 is preferred; this fallback is used only
 * when a matching NNN.bin file is available on the SD card. Indexed pixels and
 * the RGB565 palette are held in memory.
 */
struct SdMon {
  bool loaded = false;
  uint16_t w = 0, h = 0, frames = 0, frameMs = 100;
  uint8_t scale = 2;       // Integer zoom factor used when drawing.
  uint16_t palCount = 0;
  uint16_t pal[256];
  uint8_t *data = nullptr;  // Frame indices (0xFF = transparent), sized frames * w * h.

  /** Loads the requested species sprite from the SD card into memory. */
  bool load(uint16_t dexNum, bool shiny = false);
  /** Releases the loaded sprite data. */
  void unload();
};

/** Animation action identifiers used by TPK2/PMD sprites. */
enum : uint8_t {
  PMD_IDLE = 0, PMD_WALKL, PMD_WALKR, PMD_SLEEP, PMD_EAT, PMD_HURT,
  PMD_ATTACK, PMD_POSE, PMD_HOP, PMD_NOD, PMD_BREATH, PMD_SIT,
  PMD_NACTS
};

struct PmdAct {
  uint8_t w = 0, h = 0, frames = 0;
  uint8_t base = 0;  // Row after the lowest pixel, used to anchor the feet.
  uint16_t ms[24];
  const uint8_t *data = nullptr;  // Frame data in the blob, sized frames * w * h.
};

/** Multi-action PMD sprite loaded from the SD card into PSRAM. */
struct PmdMon {
  bool loaded = false;
  uint16_t palCount = 0;
  uint16_t pal[256];
  uint8_t *blob = nullptr;
  PmdAct acts[PMD_NACTS];

  /** Loads a species sprite and its animation data from the SD card. */
  bool load(uint16_t dexNum, bool shiny = false);
  /** Releases the loaded sprite data. */
  void unload();
  /** Returns whether the requested action has at least one frame. */
  bool has(uint8_t a) const { return loaded && a < PMD_NACTS && acts[a].frames > 0; }
};

/** Gallery thumbnail index loaded from thumbs.bin into PSRAM. */
struct SdThumbs {
  bool loaded = false;
  uint8_t *data = nullptr;
  uint32_t dataSize = 0;
  uint16_t count = 0;
  /** Loads and validates the thumbnail index from the SD card. */
  bool load();
  /** Releases the loaded thumbnail index. */
  void unload();
  /** Returns the encoded thumbnail for a Dex number, or null if unavailable. */
  const uint8_t *get(int16_t dex) const;
};
/** Shared gallery thumbnail index. */
extern SdThumbs thumbs;

/** Mounts the SD card using the configured one-bit SDMMC interface. */
void sdBegin(fs::FS &fs);
/** Handles a supported USB SD-card command and reports whether it was consumed. */
bool sdSerialCommand(const String &line);
/** Indicates whether the SD card is mounted and available. */
extern bool sdReady;
/** Indicates that received files may require reloading sprite data. */
extern bool sdDirty;
