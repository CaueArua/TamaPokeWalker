#include "audio.h"
#include "species_chirp.h"
#include "dex.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <Preferences.h>

// ---------------------------------------------------------------------------
// TamaPoke audio uses an ES8311 codec (DAC -> PA amplifier -> speaker) over I2S.
// ES8311 initialization is adapted from Espressif's official esp-bsp driver and
// configured for 4.096 MHz MCLK (256*fs), 16 kHz, 16-bit slave I2S. Sound
// effects are synthesized in a separate task so the game loop remains unblocked.
// ---------------------------------------------------------------------------

#define ES8311_ADDR 0x18
#define SAMPLE_RATE 16000

static I2SClass i2s;
static bool gReady = false;
static uint8_t gMode = SOUND_FULL;
static QueueHandle_t gQ = nullptr;
static volatile bool gBusy = false;
static uint32_t gLastQueuedAt[4] = {0, 0, 0, 0};
static uint32_t gLastChirpAt = 0;

enum AudioEventKind : uint8_t { AUDIO_EVENT_SFX = 0, AUDIO_EVENT_CHIRP };
struct AudioEvent {
  uint8_t kind;
  uint16_t value;
};

// ES8311 codec I2C access.
/** Writes one register on the ES8311 codec. */
static bool esW(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
/** Reads one register from the ES8311 codec. */
static uint8_t esR(uint8_t reg) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ES8311_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

// This initialization sequence is verified on the same Waveshare 1.75 board.
// It uses BCLK as the clock source and the internal DAC reference; without
// these settings the codec responds over I2C but produces no audio.
/** Configures the ES8311 for 16 kHz, 16-bit slave-I2S playback. */
static bool es8311Init() {
  Wire.beginTransmission(ES8311_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // open()
  esW(0x0D, 0xFA); esW(0x44, 0x08); esW(0x44, 0x08);  // Power up and first-write quirk.
  esW(0x01, 0x30); esW(0x02, 0x00); esW(0x03, 0x10); esW(0x16, 0x24);
  esW(0x04, 0x10); esW(0x05, 0x00); esW(0x0B, 0x00); esW(0x0C, 0x00);
  esW(0x10, 0x1F); esW(0x11, 0x7F);
  esW(0x00, 0x80); esW(0x00, 0x80);                   // Reset clock and select slave mode.
  esW(0x01, 0xBF);                                    // Use BCLK as the source, without external MCLK.
  { uint8_t r = esR(0x06); r &= ~0x20; esW(0x06, r); }  // Do not invert SCLK.
  esW(0x13, 0x10); esW(0x1B, 0x0A); esW(0x1C, 0x6A);
  esW(0x44, 0x58);                                    // Route the internal reference to the DAC.

  // config_sample(): BCLK*8 = DIG_MCLK
  esW(0x02, 0x18); esW(0x05, 0x00); esW(0x03, 0x10); esW(0x04, 0x20);
  { uint8_t r = esR(0x07); r &= 0xC0; esW(0x07, r); }
  esW(0x08, 0xFF);
  { uint8_t r = esR(0x06); r &= 0xE0; r |= 0x03; esW(0x06, r); }  // bclk_div=4

  // formato I2S 16-bit
  esW(0x09, 0x0C); esW(0x0A, 0x0C);

  // start() DAC esclavo
  esW(0x00, 0x80); esW(0x01, 0xBF); esW(0x09, 0x0C); esW(0x0A, 0x0C);
  esW(0x17, 0xBF); esW(0x0E, 0x02); esW(0x12, 0x00); esW(0x14, 0x1A);
  esW(0x0D, 0x01); esW(0x15, 0x40); esW(0x37, 0x08); esW(0x45, 0x00);

  // volumen + unmute
  esW(0x32, 0xBF);                                    // volumen DAC ~0 dB
  { uint8_t r = esR(0x31); r &= 0x9F; esW(0x31, r); }  // unmute
  return true;
}

// Compact waveform synthesizer supporting pitch slides and noise.
enum Wave : uint8_t { W_SQUARE = 0, W_TRI, W_SOFT, W_NOISE };
struct Note {
  uint16_t f, ms;
  int16_t slide;
  uint8_t vol;
  uint8_t wave;
};

#define SQ(f, ms, vol)       {f, ms, 0, vol, W_SQUARE}
#define TRI(f, ms, vol)      {f, ms, 0, vol, W_TRI}
#define SOFT(f, ms, vol)     {f, ms, 0, vol, W_SOFT}
#define NS(ms, vol)          {0, ms, 0, vol, W_NOISE}
#define SL(f, ms, to, vol, w) {f, ms, (int16_t)((to) - (f)), vol, w}
#define SIL(ms)              {0, ms, 0, 0, W_SQUARE}

static const Note N_TAP[]    = {SQ(1175, 52, 88)};
static const Note N_EAT[]    = {SOFT(523, 42, 64), SIL(12), SOFT(659, 50, 70)};
static const Note N_PLAY[]   = {SL(760, 65, 1080, 92, W_TRI), SQ(1397, 55, 86)};
static const Note N_HEART[]  = {SOFT(1047, 70, 56), SIL(18), SOFT(1319, 105, 68)};
static const Note N_HATCH[]  = {TRI(523, 70, 60), TRI(659, 70, 64), TRI(784, 95, 68), SL(880, 190, 1320, 72, W_TRI)};
static const Note N_EVOLVE[] = {SL(392, 100, 560, 58, W_TRI), SL(523, 100, 740, 62, W_TRI), SL(659, 110, 960, 66, W_TRI), SL(880, 210, 1480, 76, W_SOFT)};
static const Note N_MEDAL[]  = {TRI(784, 60, 66), SIL(22), TRI(1047, 68, 72), SIL(20), SL(1175, 210, 1568, 78, W_TRI)};
static const Note N_DENY[]   = {SL(330, 120, 230, 70, W_SQUARE), SL(220, 150, 160, 64, W_SQUARE)};
static const Note N_BYE[]    = {SOFT(784, 130, 58), SOFT(659, 140, 55), SL(523, 260, 392, 54, W_SOFT)};
static const Note N_LEVEL[]  = {TRI(784, 65, 64), TRI(1047, 80, 70), SOFT(1319, 130, 70)};
static const Note N_BATTLE_WIN[]   = {TRI(659, 58, 66), TRI(784, 58, 68), TRI(988, 80, 72), SL(1175, 170, 1568, 76, W_TRI)};
static const Note N_BATTLE_LOSS[]  = {SL(392, 140, 330, 66, W_SOFT), SL(330, 140, 247, 62, W_SOFT), SOFT(196, 220, 56)};
static const Note N_CATCH_OK[]     = {TRI(784, 55, 68), TRI(988, 65, 72), SL(1175, 180, 1568, 78, W_TRI)};
static const Note N_CATCH_FAIL[]   = {NS(55, 50), SL(523, 80, 392, 64, W_SQUARE), SIL(16), SL(392, 180, 247, 62, W_SOFT)};
static const Note N_DAILY_GOAL[]   = {TRI(1175, 50, 68), SIL(22), TRI(1568, 70, 74), SOFT(1760, 95, 68)};
static const Note N_EVENT_SPARKLE[] = {NS(35, 36), TRI(1568, 42, 56), TRI(1976, 62, 60), SIL(18), TRI(1760, 56, 54)};
static const Note N_REST[]         = {SL(523, 125, 392, 48, W_SOFT), SOFT(330, 170, 42)};
static const Note N_COUNTER[]      = {SL(784, 75, 1175, 62, W_TRI), SIL(16), SQ(1568, 70, 74), NS(40, 42)};
static const Note N_MENU[]         = {TRI(988, 56, 84), SQ(1319, 62, 90)};
static const Note N_GAME_START[]   = {TRI(659, 58, 72), TRI(880, 64, 78), SQ(1175, 74, 82)};
static const Note N_BALL_BOUNCE[]  = {SL(820, 42, 520, 72, W_SQUARE)};
static const Note N_BALL_MISS[]    = {NS(55, 56), SL(360, 110, 210, 68, W_SOFT)};
static const Note N_MEMO_STEP[]    = {SQ(1047, 54, 68)};
static const Note N_MEMO_PAD_0[]   = {SOFT(349, 82, 76)};
static const Note N_MEMO_PAD_1[]   = {TRI(523, 82, 76)};
static const Note N_MEMO_PAD_2[]   = {TRI(784, 82, 76)};
static const Note N_MEMO_PAD_3[]   = {SQ(1047, 82, 76)};
static const Note N_ATTACK_QUICK[] = {SL(980, 42, 1320, 90, W_TRI), SQ(1760, 38, 82)};
static const Note N_ATTACK_HEAVY[] = {NS(36, 46), SL(330, 74, 700, 92, W_SQUARE), SQ(880, 52, 86)};
static const Note N_ENEMY_HIT[]    = {SL(300, 70, 190, 82, W_SQUARE), NS(38, 44)};
static const Note N_EFFECTIVE[]    = {TRI(988, 48, 82), TRI(1319, 54, 90), SQ(1760, 64, 86)};
static const Note N_WEAK_HIT[]     = {SOFT(420, 70, 58), SOFT(360, 90, 50)};
static const Note N_MINIGAME_OK[]  = {SL(1047, 46, 1568, 88, W_TRI), TRI(1760, 42, 78)};
static const Note N_MINIGAME_BAD[] = {NS(42, 52), SL(300, 95, 180, 70, W_SOFT)};
static const Note N_LOW_HP[]       = {SQ(740, 70, 74), SIL(38), SQ(740, 70, 74)};
static const Note N_EXPEDITION_START[] = {TRI(523, 52, 64), TRI(659, 62, 70), SL(784, 115, 1047, 72, W_TRI)};
static const Note N_EXPEDITION_FOUND[] = {TRI(784, 55, 70), TRI(1047, 58, 76), TRI(1319, 65, 78), SOFT(1568, 130, 72)};
static const Note N_EXPEDITION_CLAIM[] = {SOFT(988, 55, 66), TRI(1319, 70, 74), SL(1568, 115, 1976, 76, W_TRI)};
static const Note N_ITEM_USE[] = {SOFT(659, 48, 62), SL(784, 95, 1175, 70, W_TRI)};

struct SfxDef { const Note *n; uint8_t len; };
static const SfxDef SFX[SFX_COUNT] = {
  {N_TAP, 1}, {N_EAT, 3}, {N_PLAY, 2}, {N_HEART, 2}, {N_HATCH, 4},
  {N_EVOLVE, 4}, {N_MEDAL, 5}, {N_DENY, 2}, {N_BYE, 3}, {N_LEVEL, 3},
  {N_BATTLE_WIN, 4}, {N_BATTLE_LOSS, 3}, {N_CATCH_OK, 3}, {N_CATCH_FAIL, 4},
  {N_DAILY_GOAL, 4}, {N_EVENT_SPARKLE, 5}, {N_REST, 2}, {N_COUNTER, 4},
  {N_MENU, 2}, {N_GAME_START, 3}, {N_BALL_BOUNCE, 1}, {N_BALL_MISS, 2}, {N_MEMO_STEP, 1},
  {N_MEMO_PAD_0, 1}, {N_MEMO_PAD_1, 1}, {N_MEMO_PAD_2, 1}, {N_MEMO_PAD_3, 1},
  {N_ATTACK_QUICK, 2}, {N_ATTACK_HEAVY, 3}, {N_ENEMY_HIT, 2}, {N_EFFECTIVE, 3},
  {N_WEAK_HIT, 2}, {N_MINIGAME_OK, 2}, {N_MINIGAME_BAD, 2}, {N_LOW_HP, 3},
  {N_EXPEDITION_START, 3}, {N_EXPEDITION_FOUND, 4}, {N_EXPEDITION_CLAIM, 3}, {N_ITEM_USE, 2},
};

static const uint8_t SFX_MIN_MODE[SFX_COUNT] = {
  SOUND_FULL, // TAP: nur "viel"
  SOUND_MED,  // EAT
  SOUND_FULL, // PLAY: kleine Punkte/Klicks nur "viel"
  SOUND_MED,  // HEART
  SOUND_LOW,  // HATCH: major event.
  SOUND_LOW,  // EVOLVE
  SOUND_LOW,  // MEDAL
  SOUND_LOW,  // DENY: wichtiges Feedback auch bei wenig
  SOUND_LOW,  // BYE
  SOUND_LOW,  // LEVEL
  SOUND_LOW,  // BATTLE_WIN
  SOUND_LOW,  // BATTLE_LOSS
  SOUND_LOW,  // CATCH_OK
  SOUND_LOW,  // CATCH_FAIL
  SOUND_LOW,  // DAILY_GOAL
  SOUND_MED,  // EVENT_SPARKLE
  SOUND_MED,  // REST
  SOUND_MED,  // COUNTER
  SOUND_FULL, // MENU
  SOUND_MED,  // GAME_START
  SOUND_FULL, // BALL_BOUNCE
  SOUND_FULL, // BALL_MISS
  SOUND_FULL, // MEMO_STEP
  SOUND_FULL, // MEMO_PAD_0
  SOUND_FULL, // MEMO_PAD_1
  SOUND_FULL, // MEMO_PAD_2
  SOUND_FULL, // MEMO_PAD_3
  SOUND_FULL, // ATTACK_QUICK
  SOUND_FULL, // ATTACK_HEAVY
  SOUND_FULL, // ENEMY_HIT
  SOUND_MED,  // EFFECTIVE
  SOUND_FULL, // WEAK_HIT
  SOUND_MED,  // MINIGAME_OK
  SOUND_MED,  // MINIGAME_BAD
  SOUND_LOW,  // LOW_HP
  SOUND_MED,  // EXPEDITION_START
  SOUND_LOW,  // EXPEDITION_FOUND
  SOUND_MED,  // EXPEDITION_CLAIM
  SOUND_MED,  // ITEM_USE
};

static int16_t buf[256 * 2];  // Interleaved stereo samples (left = right).

static uint16_t noiseState = 0xACE1;
/** Advances the noise generator and returns one signed sample. */
static int16_t nextNoise() {
  noiseState = (uint16_t)((noiseState >> 1) ^ (-(noiseState & 1u) & 0xB400u));
  return (noiseState & 1) ? 1 : -1;
}

/** Produces one sample for the requested waveform and phase. */
static int16_t oscSample(uint8_t wave, int phase, int period, int16_t amp) {
  if (wave == W_NOISE) return (int16_t)(nextNoise() * amp);
  if (period <= 1) return 0;
  int p = phase % period;
  if (wave == W_TRI) {
    int half = period / 2;
    int v = (p < half) ? (-amp + (2 * amp * p) / half) : (amp - (2 * amp * (p - half)) / (period - half));
    return (int16_t)v;
  }
  if (wave == W_SOFT) {
    int half = period / 2;
    int q = (p < half) ? p : period - p;
    int v = (2 * amp * q) / half - amp;
    return (int16_t)(v * 3 / 4);
  }
  return (p < period / 2) ? amp : -amp;
}

/** Returns the output gain percentage for the selected sound mode. */
static uint8_t modeGainPct() {
  switch (gMode) {
    case SOUND_LOW: return 58;
    case SOUND_MED: return 82;
    case SOUND_FULL: return 118;
    default: return 0;
  }
}

/** Returns whether a sound effect is allowed at the lowest volume mode. */
static bool criticalSfx(uint8_t id) {
  return id < SFX_COUNT && SFX_MIN_MODE[id] == SOUND_LOW;
}

/** Synthesizes and writes one note, using short fades to prevent clicks. */
static void playTone(const Note &note) {
  uint16_t f = note.f;
  uint16_t ms = note.ms;
  int total = SAMPLE_RATE * ms / 1000;
  int done = 0;
  int phase = 0;
  const int16_t maxAmp = (int16_t)(9200L * note.vol * modeGainPct() / 10000);
  while (done < total) {
    int n = total - done; if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int16_t s = 0;
      int idx = done + i;
      if (note.wave == W_NOISE || f) {
        uint16_t curF = f;
        if (note.slide && total > 1) {
          int32_t v = (int32_t)note.f + ((int32_t)note.slide * idx) / total;
          curF = (v < 20) ? 20 : (uint16_t)v;
        }
        int period = curF ? (SAMPLE_RATE / curF) : 0;
        if (period < 2) period = 2;
        int16_t amp = maxAmp;
        if (idx < 64) s = (int16_t)(s * idx / 64);                 // ataque
        s = oscSample(note.wave, phase, period, amp);
        if (idx < 64) s = (int16_t)(s * idx / 64);
        else if (idx > total - 96) s = (int16_t)(s * (total - idx) / 96);
        phase++;
      }
      buf[i * 2] = s; buf[i * 2 + 1] = s;
    }
    i2s.write((uint8_t *)buf, n * 4);
    done += n;
  }
}

/** Builds and plays the synthetic chirp profile for a species. */
static void playSpeciesChirp(uint16_t dex) {
  SpeciesChirpProfile profile{};
  if (!speciesChirpProfile(dex, &profile)) return;
  for (uint8_t i = 0; i < profile.count; i++) {
    const SpeciesChirpNote &src = profile.notes[i];
    uint8_t volume = src.volume;
    if (gMode == SOUND_FULL && volume < 90) volume = (uint8_t)(volume + 10);
    Note note = { src.frequency, src.durationMs, src.slide, volume, src.wave };
    playTone(note);
  }
}

/** Consumes queued audio events and plays their sounds on the audio task. */
static void audioTask(void *) {
  AudioEvent event;
  for (;;) {
    if (!xQueueReceive(gQ, &event, portMAX_DELAY) || !gReady) continue;
    bool isSfx = event.kind == AUDIO_EVENT_SFX && event.value < SFX_COUNT;
    bool isChirp = event.kind == AUDIO_EVENT_CHIRP && event.value >= 1 && event.value <= DEX_COUNT && gMode >= SOUND_MED;
    if (isSfx && gMode < SFX_MIN_MODE[event.value]) continue;
    if (isSfx || isChirp) {
      gBusy = true;
      digitalWrite(PA, HIGH);  // Enable the amplifier.
      delay(8);                // Allow the amplifier to start.
      if (isSfx) {
        const SfxDef &d = SFX[event.value];
        for (uint8_t i = 0; i < d.len; i++) playTone(d.n[i]);
      } else {
        playSpeciesChirp(event.value);
      }
      delay(gMode == SOUND_FULL ? 90 : 60);  // Let the DMA queue drain before stopping.
      digitalWrite(PA, LOW);                 // Disable the amplifier between sounds.
      gBusy = false;
    }
  }
}

void audioBegin() {
  // Start I2S first so the codec can lock to its clock.
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);   // Keep the amplifier off until the audio task plays a sound.

  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("I2S begin fallo");
    return;
  }
  if (!es8311Init()) { Serial.println("ES8311 no responde (audio off)"); return; }

  Preferences p;
  p.begin("tamapoke", true);
  if (p.isKey("sndm")) {
    gMode = p.getUChar("sndm", SOUND_FULL);
  } else {
    gMode = p.getBool("snd", true) ? SOUND_FULL : SOUND_OFF;
  }
  p.end();
  if (gMode > SOUND_FULL) gMode = SOUND_FULL;

  gReady = true;
  gQ = xQueueCreate(32, sizeof(AudioEvent));
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 1, nullptr, 0);
  sfxPlay(SFX_HATCH);  // Play a startup jingle to confirm audio is working.
}

void sfxPlay(uint8_t id) {
  if (!gReady || gMode == SOUND_OFF || !gQ || id >= SFX_COUNT || gMode < SFX_MIN_MODE[id]) return;

  // Keep the sound modes distinct: full plays everything, medium filters
  // rapid repeats, and low is reserved for major events and clear warnings.
  uint32_t now = millis();
  if (gMode == SOUND_MED && !criticalSfx(id)) {
    if (now - gLastQueuedAt[gMode] < 180UL) return;
  } else if (gMode == SOUND_LOW) {
    if (now - gLastQueuedAt[gMode] < 650UL && !criticalSfx(id)) return;
  }
  gLastQueuedAt[gMode] = now;
  AudioEvent event = { AUDIO_EVENT_SFX, id };
  xQueueSend(gQ, &event, gMode == SOUND_FULL ? pdMS_TO_TICKS(28) : 0);
}

void audioSetEnabled(bool on) {
  audioSetMode(on ? SOUND_FULL : SOUND_OFF);
}

bool audioEnabled() { return gMode != SOUND_OFF; }

void audioSetMode(uint8_t mode) {
  if (mode > SOUND_FULL) mode = SOUND_FULL;
  gMode = mode;
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("sndm", gMode);
  p.putBool("snd", gMode != SOUND_OFF);
  p.end();
}

uint8_t audioMode() { return gMode; }

bool audioBusy() {
  return gBusy || (gQ && uxQueueMessagesWaiting(gQ) > 0);
}

void speciesChirpPlay(int16_t dex, bool force) {
  if (!gReady || !gQ || gMode < SOUND_MED || dex < 1 || dex > DEX_COUNT) return;
  uint32_t now = millis();
  uint32_t cooldown = force ? 220UL : 800UL;
  if (gLastChirpAt && now - gLastChirpAt < cooldown) return;
  gLastChirpAt = now;
  AudioEvent event = { AUDIO_EVENT_CHIRP, (uint16_t)dex };
  xQueueSend(gQ, &event, gMode == SOUND_FULL ? pdMS_TO_TICKS(28) : 0);
}
