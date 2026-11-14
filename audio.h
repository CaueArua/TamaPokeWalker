#pragma once
#include <stdint.h>

/**
 * Sound effects played through the non-blocking audio queue.
 * The values are ordered to match the sound-effect table in audio.cpp.
 */
enum Sfx : uint8_t {
  SFX_TAP = 0,  // Touch or button press.
  SFX_EAT,      // Feeding.
  SFX_PLAY,     // Minigame score or hit.
  SFX_HEART,    // Affection or petting.
  SFX_HATCH,    // Egg hatching.
  SFX_EVOLVE,   // Evolution.
  SFX_MEDAL,    // Medal or milestone.
  SFX_DENY,     // Rejected action.
  SFX_BYE,      // Farewell.
  SFX_LEVEL,    // Level up.
  SFX_BATTLE_WIN,
  SFX_BATTLE_LOSS,
  SFX_CATCH_OK,
  SFX_CATCH_FAIL,
  SFX_DAILY_GOAL,
  SFX_EVENT_SPARKLE,
  SFX_REST,
  SFX_COUNTER,
  SFX_MENU,
  SFX_GAME_START,
  SFX_BALL_BOUNCE,
  SFX_BALL_MISS,
  SFX_MEMO_STEP,
  SFX_MEMO_PAD_0,
  SFX_MEMO_PAD_1,
  SFX_MEMO_PAD_2,
  SFX_MEMO_PAD_3,
  SFX_ATTACK_QUICK,
  SFX_ATTACK_HEAVY,
  SFX_ENEMY_HIT,
  SFX_EFFECTIVE,
  SFX_WEAK_HIT,
  SFX_MINIGAME_OK,
  SFX_MINIGAME_BAD,
  SFX_LOW_HP,
  SFX_EXPEDITION_START,
  SFX_EXPEDITION_FOUND,
  SFX_EXPEDITION_CLAIM,
  SFX_ITEM_USE,
  SFX_COUNT
};

enum SoundMode : uint8_t {
  SOUND_OFF = 0,
  SOUND_LOW,
  SOUND_MED,
  SOUND_FULL,
};

/** Initializes the ES8311 codec, I2S output, amplifier, and audio task. */
void audioBegin();
/** Queues a sound effect without blocking the main loop. */
void sfxPlay(uint8_t id);
/** Enables or disables sound playback. */
void audioSetEnabled(bool on);
/** Returns whether sound playback is enabled. */
bool audioEnabled();
/** Selects the output volume mode. */
void audioSetMode(uint8_t mode);
/** Returns the currently selected output volume mode. */
uint8_t audioMode();
/** Returns whether the audio task is currently playing a sound. */
bool audioBusy();
/** Plays the synthetic chirp assigned to a species. */
void speciesChirpPlay(int16_t dex, bool force = false);
