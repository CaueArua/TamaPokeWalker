#pragma once

#include <stdint.h>

/** Waveforms available to the synthetic species-chirp generator. */
enum ChirpWave : uint8_t {
  CHIRP_SQUARE = 0,
  CHIRP_TRI,
  CHIRP_SOFT,
  CHIRP_NOISE,
};

struct SpeciesChirpNote {
  uint16_t frequency;
  uint16_t durationMs;
  int16_t slide;
  uint8_t volume;
  uint8_t wave;
};

struct SpeciesChirpProfile {
  SpeciesChirpNote notes[4];
  uint8_t count;
};

/** Returns the synthetic chirp profile for a species when one is defined. */
bool speciesChirpProfile(int16_t dex, SpeciesChirpProfile *out);
