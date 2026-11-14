// TamaPoke - tamagotchi pixel art inspirado en la gen 1
// para Waveshare ESP32-S3-Touch-AMOLED-1.75
//
// Librerias (Library Manager o repo de Waveshare):
//   - "GFX Library for Arduino" (moononournation), con soporte CO5300 QSPI
//   - "SensorLib" (Lewis He), driver tactil CST9217
//
// Placa: ESP32S3 Dev Module | Flash 16MB | PSRAM: OPI PSRAM | USB CDC On Boot: Enabled
//
// Los sprites y la tabla de especies se generan con tools/sprites.py (emit).

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "Arduino_GFX_Library.h"
#include "TouchDrvCSTXXX.hpp"
#include "pin_config.h"
#include "species.h"
#include "dex.h"
#include "pet.h"
#include "sdmon.h"
#include "rtcbat.h"
#include "i18n.h"
#include "audio.h"
#include "battle.h"
#include "time_utils.h"
#include "dayphase.h"
#include "imu.h"
#include "src/ui/party_screen.h"
#include "src/ui/dex_screen.h"
#include "src/ui/strength_screen.h"
#include "src/ui/battle_screen.h"
#include "src/ui/game_ball_screen.h"
#include "src/ui/game_catch_screen.h"
#include "src/ui/game_memo_screen.h"
#include "src/ui/game_clean_screen.h"
#include "src/ui/game_type_screen.h"
#include "src/ui/game_screen_common.h"
#include "src/ui/pet_card_profile_screen.h"
#include "src/ui/pet_card_page.h"
#include "src/ui/pet_card_personality_screen.h"
#include "src/ui/pet_card_daily_screen.h"
#include "src/ui/pet_card_box_screen.h"
#include "src/ui/pet_card_stats_screen.h"
#include "src/ui/pet_card_medals_screen.h"
#include "src/ui/pet_card_progress_screen.h"
#include "src/ui/pet_card_expedition_screen.h"
#include "src/ui/pet_card_steps_screen.h"
#include "src/ui/clock_screen.h"
#include "src/ui/keyboard_screen.h"
#include "src/ui/help_screen.h"
#include "src/ui/starter_screen.h"

// Version del firmware. Subir este numero en cada release (y manifest.json para
// el instalador web). Se muestra en la pantalla de ajustes y por serie al arrancar.
#if defined(TAMAPOKE_LOCAL_TEST) && defined(TAMAPOKE_GEN3_RELEASE)
#error "TAMAPOKE_LOCAL_TEST and TAMAPOKE_GEN3_RELEASE are mutually exclusive"
#endif
#if DEX_COUNT > 251 && !defined(TAMAPOKE_LOCAL_TEST) && !defined(TAMAPOKE_GEN3_RELEASE)
#error "Gen-3 source requires an explicit local-test or release build profile"
#endif
#ifdef TAMAPOKE_LOCAL_TEST
#define FW_VERSION "1.36.2-hoenn-walk-local"
#elif defined(TAMAPOKE_GEN3_RELEASE)
#define FW_VERSION "1.36.2"
#else
#define FW_VERSION "1.35.3-soft-step"
#endif

const char *firmwareVersion() { return FW_VERSION; }

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *panel = new Arduino_CO5300(
  bus, LCD_RESET, 0 /*rotation*/, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);
// Framebuffer completo en PSRAM: dibujamos todo y hacemos flush() (sin parpadeo)
Arduino_Canvas *gfx = new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

TouchDrvCST92xx touch;
Pet pet;

// sprite animado de la SD para la especie actual (si existe el archivo)
SdMon mon;          // sprite B/N (respaldo y minijuego si no hay PMD)
PmdMon pmd;         // sprite PMD multi-accion (pantalla principal)
PmdMon evoPmd;      // forma anterior, solo durante el parpadeo de evolucion
PmdMon wildPmd;     // rival salvaje en la pantalla de combate
int16_t monFor = -2;
bool monShinyFor = false;

// comportamiento del bicho en pantalla
struct {
  uint8_t mode = 0;     // 0 idle, 1 paseo, 2 gesto one-shot
  uint8_t act = PMD_IDLE;
  uint32_t t0 = 0;      // inicio de la animacion en curso
  uint32_t until = 0;   // fin del estado actual
  float x = 233, targetX = 233;
} beh;
#define PET_GROUND 304  // linea de suelo de la mascota
// DEX

// galeria pokedex
bool screenOff = false;       // pulsacion corta del boton PWR
bool cardOpen = false;        // ficha del bicho (deslizar vertical)
bool kbOpen = false;          // teclado para renombrar al bicho
char nameBuf[12] = "";
uint8_t nameLen = 0;
uint8_t cardPage = 0;         // 0 perfil ... 7 expedicion, 8 pasos/trail
bool expeditionTrainChoiceOpen = false;
bool clockOpen = false;       // pantalla de ajuste de hora (deslizar abajo)
int clockH = 12, clockM = 0;  // hora en edicion
bool powerSave = false;       // ahorro opcional: off por defecto
bool helpOpen = false;
uint8_t helpPage = 0;
bool uiDirty = true;
bool cardDirty = true;
bool clockDirty = true;
bool helpDirty = true;
bool keyboardDirty = true;
bool gameMenuDirty = true;
bool battleDirty = true;

int8_t watermarkDrawnSlot = -1;

// escena de bano: espuma sobre el bicho y limpieza al reventar
uint32_t bathUntil = 0;
bool bathPending = false;
struct { int16_t x, y; uint8_t r, ph; } bubbles[14];
uint32_t feedMenuUntil = 0;   // selector de comida abierto hasta este millis
uint32_t nextAmbientSoundAt = 0;

// minijuego "toques": mantener la pokeball en el aire
bool gameOpen = false;
bool gameMenuOpen = false;
uint8_t gameMode = 0;  // 0 ball, 1 catch, 2 memo
uint32_t gameOverUntil = 0;
float ballX, ballY, ballVX, ballVY, gamePetX;
uint8_t gameScore, gameMisses;
float hitX, hitY;             // ultimo golpe (anillo de impacto)
uint32_t hitTime = 0;
uint32_t ballLastHitAt = 0;
bool gameNewHi = false;
uint8_t gameGain = 0;
uint32_t catchUntil = 0, catchTargetUntil = 0;
int16_t catchX = 0, catchY = 0;
uint8_t catchIcon = 0;
uint8_t memoSeq[14] = { 0 };
uint8_t memoLen = 0, memoShow = 0, memoInput = 0, memoRounds = 0;
uint32_t memoNextAt = 0;
bool memoShowing = false;
int8_t memoActivePad = -1, memoFlashPad = -1, memoHintPad = -1;
bool memoFlashGood = false;
uint32_t memoFlashUntil = 0, memoFailUntil = 0, memoTurnUntil = 0;
int16_t cleanX[4] = { 0 }, cleanY[4] = { 0 };
bool cleanAlive[4] = { false };
uint32_t cleanUntil = 0, cleanSpawnAt = 0;
uint8_t cleanActive = 0;
uint8_t typeEnemy = TYPE_GRASS;
uint8_t typeChoice[3] = { TYPE_FIRE, TYPE_WATER, TYPE_GRASS };
uint8_t typeCorrect = 0;
uint32_t typeUntil = 0;

bool battleOpen = false;
bool battleResolved = false;
int16_t battleDex = 0;
uint8_t battleLevel = 1;
BattleStats battlePlayer = {};
BattleStats battleEnemy = {};
BattleResult battleResult = {};
BattleRuntime battleRun = {};
BattleTurnResult battleTurn = {};
BattleAction battleLastAction = BATTLE_ATTACK;
BattleReward battleReward = {};
char battleMsg[28] = "";
uint32_t battleAttackMenuUntil = 0;
bool battleCatchOffered = false;
bool battleCatchTried = false;
bool battleCatchDone = false;
bool battleCatchSuccess = false;
bool battleRespectCatch = false;
uint8_t battleCatchChance = 0;
bool battleLowHpWarned = false;

#define WILD_COOLDOWN_MS (20UL * 60UL * 1000UL)
#define WILD_PROMPT_MS 20000UL
#define WILD_CHECK_MS 60000UL
uint32_t wildPromptUntil = 0;
uint32_t nextWildEligible = 0;
uint32_t lastWildCheck = 0;
int16_t wildPromptDex = 0;
uint8_t wildPromptLevel = 1;
bool wildPromptShiny = false;
bool battleShiny = false;

#define PET_EVENT_COOLDOWN_MS (15UL * 60UL * 1000UL)
#define PET_EVENT_PROMPT_MS 18000UL
#define PET_EVENT_CHECK_MS 60000UL
uint32_t petEventUntil = 0;
uint32_t nextPetEventEligible = 0;
uint32_t lastPetEventCheck = 0;
uint8_t petEventType = PET_EVENT_BERRY;
uint32_t petEventFeedbackUntil = 0;
char petEventMsg[18] = "";
uint32_t morningUntil = 0;
uint32_t eggWiggleUntil = 0;

int sceneHour();
uint8_t currentDayPhase();
bool mainScreenReadyForShake();
void maybeOfferMorning(uint32_t now);
uint32_t pmdActTotalMs(const PmdAct &a);
bool watermarkNeedsRedraw();
void flushStampedFrame();
void startBattleWith(int16_t forcedDex, uint8_t forcedLevel, int8_t forcedShiny = -1);
void startBattle();

// las 9 especies con sprite propio en flash (respaldo sin SD): dex -> indice
int flashIdxForDex(int16_t dex) {
  static const int8_t IDX[10] = { -1, 3, 4, 5, 0, 1, 2, 6, 7, 8 };
  return (dex >= 1 && dex <= 9) ? IDX[dex] : -1;
}

#define CX 233  // centro de la pantalla redonda
#define CY 233
#define PET_CY 202  // centro vertical del sprite

static const uint16_t INK_K = 0x18C4;  // spriteColor('k')

void loadPowerSave() {
  Preferences p;
  p.begin("tamapoke", true);
  powerSave = p.getBool("psave", false);
  p.end();
}

void setPowerSave(bool on) {
  powerSave = on;
  Preferences p;
  p.begin("tamapoke", false);
  p.putBool("psave", powerSave);
  p.end();
}

// botones de icono siguiendo el arco inferior de la pantalla redonda
// (los exteriores van mas altos para no salirse del circulo)
struct Btn {
  int16_t cx, cy;
  const char *const *icon;
};
Btn buttons[4] = {
  { 140, 390, SPR_ICON_FOOD },   // comer
  { 202, 404, SPR_ICON_PLAY },   // jugar
  { 264, 404, SPR_ICON_LIGHT },  // luz
  { 326, 390, SPR_ICON_CLEAN },  // bano
};
#define BTN_HALF 26  // boton de 52x52
#define BTN_HIT 36   // radio tactil (un poco mas generoso)

// grietas del huevo (pixeles 'k' sobre el sprite)
static const uint8_t CRACK1[][2] = { {15,8},{16,9},{15,10} };
static const uint8_t CRACK2[][2] = { {11,13},{12,14},{11,15},{20,12},{19,13},{20,14} };
// estrellas del modo noche
static const uint16_t STARS[][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };

bool wasPressed = false;
// boton-CTA de evolucion (centrado, mitad de pantalla)
#define EVO_BTN_W 256
#define EVO_BTN_H 64
#define EVO_BTN_X (CX - EVO_BTN_W / 2)
#define EVO_BTN_Y 172
// boton-CTA de despedida (mas ancho: lleva el nombre + frase)
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
// el CST9217 avisa por el pin INT cuando hay datos tactiles; lo usamos para no
// leer el bus I2C mientras el chip esta dormido (esa lectura se colgaba ~1s)
volatile bool gTouchIrq = false;
void IRAM_ATTR touchIsr() { gTouchIrq = true; }
uint32_t lastRender = 0;
uint32_t lastLoopStart = 0;
uint32_t perfRenderLastMs = 0, perfRenderMaxMs = 0;
uint32_t perfLoopLastMs = 0, perfLoopMaxMs = 0;
uint32_t perfRenderCount = 0, perfRenderSkipCount = 0;
// proteccion del AMOLED: atenuado por inactividad
uint32_t lastInteract = 0;
uint8_t dimStage = 0;        // 0 despierto, 1 atenuado (90s), 2 casi apagado (5min)
bool swallowGesture = false; // el toque que despierta no acciona nada
uint32_t ignoreTouchUntil = 0;
uint32_t holdStart = 0;     // pulsacion larga sobre el bicho
uint32_t confirmUntil = 0;  // dialogo "soltar?" activo hasta este millis
uint8_t choiceKind = 0;     // 0 ninguno, 1 evolucion, 2 despedida, 3 objetivo de evolucion
uint32_t choiceUntil = 0;   // se cierra solo a este millis
int16_t tX0, tY0, tXl, tYl; // gesto en curso (inicio y ultima posicion)
uint32_t tStart = 0;
bool holdFired = false;

void markUiDirty() {
  uiDirty = true;
  cardDirty = true;
  clockDirty = true;
  helpDirty = true;
  keyboardDirty = true;
  gameMenuDirty = true;
  battleDirty = true;
  if (activeScreen) activeScreen->markDirty();
}

void lockTouchBrief(uint16_t ms = 160) {
  uint32_t until = millis() + ms;
  if (until > ignoreTouchUntil) ignoreTouchUntil = until;
  wasPressed = false;
  swallowGesture = true;
}

const char *screenName() {
  if (pet.awaitingStarter()) return "starter";
  if (dexScreen.isOpen()) return dexScreen.isDetail() ? "gallery-detail" : "gallery-grid";
  if (gameOpen) return "game";
  if (strengthTrainingScreen.isOpen()) return "sack";
  if (battleOpen) return battleResolved ? "battle-result" : "battle";
  if (kbOpen) return "keyboard";
  if (helpOpen) return "help";
  if (clockOpen) return "settings";
  if (cardOpen) return "card";
  if (gameMenuOpen) return "game-menu";
  return screenOff ? "screen-off" : "main";
}

bool staticScreenClean() {
  if (watermarkNeedsRedraw()) return false;
  if (battleOpen && battleResolved) return !battleDirty;
  if (kbOpen) return !keyboardDirty;
  if (helpOpen) return !helpDirty;
  if (clockOpen) return !clockDirty;
  if (cardOpen) return !cardDirty;
  if (gameMenuOpen) return !gameMenuDirty;
  if (activeScreen) return activeScreen->isStaticClean();
  return false;
}

void setup() {
  Serial.setRxBufferSize(8192);  // la transferencia a SD llega en bloques de 2 KB
  Serial.begin(115200);
  // CRITICO: sin esto, Serial.print BLOQUEA el juego cuando no hay un
  // monitor serie abierto en el host (el bufer TX del USB CDC se llena
  // y nadie lo vacia) -> con timeout 0 los mensajes se descartan
  Serial.setTxTimeoutMs(0);
  Serial.printf("TamaPoke fw v%s\n", FW_VERSION);
  loadLang();  // idioma guardado (ES por defecto)
  Wire.begin(IIC_SDA, IIC_SCL);
  // CST9217 (tactil), AXP2101 (PMU) y PCF85063 (RTC) comparten este bus I2C.
  // Red de seguridad para PMU/RTC (SensorLib NO respeta este timeout en el
  // tactil; el cuelgue del tactil dormido se resuelve gateando por INT, ver
  // handleTouch).
  Wire.setTimeOut(50);

  // CRITICO: encender la alimentacion del panel (BLDO1=OLED VDD 3.3V) ANTES de
  // inicializar el display. Si el PMU se reseteo (drenaje total), este rail
  // queda OFF y la pantalla se ve negra aunque el resto de la placa funcione.
  pmuEnablePanel();

  // QSPI a 80MHz (por defecto 40): el flush del framebuffer es el cuello de
  // botella del fps (~56ms a 40MHz). Si el panel mostrara basura, bajar a 40M.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");
  panel->setBrightness(180);

  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {  // a veces falla al primer intento
    touchOk = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 no detectado");
  // begin() deja el chip en modo comando (lee la identidad y no sale);
  // hace falta un reset por hardware para que vuelva a reportar toques
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);  // el panel esta montado girado 180 grados
  // INT activo-bajo: salta cuando hay datos. Gatea las lecturas I2C (ver loop)
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);

  pet.begin();
  if (pet.awaitingStarter()) setScreen(&starterScreen);
  Serial.printf("SAVE %s spec=%d lv=%u wins=%u losses=%u streak=%u\n",
                pet.saveLoadedFromNvs ? "loaded" : "created",
                pet.speciesId, pet.level(), pet.battleWins, pet.battleLosses, pet.battleStreak);
  sdBegin();
  thumbs.load();

  // reloj real: aplica el tiempo que estuvo apagado
  rtcBegin();
  batBegin();
  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    rtcSetEpoch(1767225600UL);  // RTC virgen: semilla (la hora absoluta da igual,
    e = rtcEpoch();             // solo importan las diferencias)
    Serial.println("RTC sin hora: sembrado, sin progresion offline esta vez");
  }
  pet.syncClock(e);
  loadPowerSave();
  imuBegin();

  audioBegin();  // ES8311 + I2S + amplificador (suena un jingle de arranque)

  lastInteract = millis();
}

// carga/descarga el sprite de SD cuando cambia la especie
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  sdDirty = false;
  monFor = pet.speciesId;
  monShinyFor = pet.shiny;
  mon.unload();
  pmd.unload();
  beh.x = beh.targetX = 233;
  beh.mode = 0;
  beh.until = 0;
  if (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    pmd.load(pet.speciesId, pet.shiny);          // principal: PMD
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // respaldo: B/N
  }
}

bool mainScreenReadyForAmbientSound() {
  if (audioMode() != SOUND_FULL || screenOff || dimStage > 0) return false;
  if (pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (activeScreen || battleOpen || gameOpen || gameMenuOpen || strengthTrainingScreen.isOpen() || cardOpen || dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil || petEventUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}


void maybePlayAmbientSound(uint32_t now) {
  if (!mainScreenReadyForAmbientSound()) {
    nextAmbientSoundAt = now + 9000;
    return;
  }
  if (nextAmbientSoundAt == 0) nextAmbientSoundAt = now + 8000 + random(8000);
  if (deadlineActive(now, nextAmbientSoundAt)) return;
  uint8_t r = (uint8_t)random(3);
  sfxPlay(r == 0 ? SFX_HEART : (r == 1 ? SFX_EVENT_SPARKLE : SFX_MENU));
  nextAmbientSoundAt = now + 8000 + random(8000);
}

uint16_t renderIntervalMs() {
  if (screenOff) return 5000;
  if (battleOpen) return battleResolved ? 320 : 190;
  if (gameOpen || strengthTrainingScreen.isOpen()) return 115;
  if (dexScreen.isOpen() || cardOpen || kbOpen || clockOpen || helpOpen || gameMenuOpen) return powerSave ? 650 : 320;
  if (!powerSave) return 100;
  if (dimStage >= 2) return 650;
  if (dimStage >= 1) return 350;
  return 180;
}

bool lightSleepAllowed(uint32_t now) {
  if (!powerSave || usbPresent() || audioBusy() || Serial.available()) return false;
  if (!screenOff && dimStage == 0) return false;
  if (!screenOff && now - lastInteract < 1500UL) return false;
  if (wasPressed || gTouchIrq || deadlineActive(now, ignoreTouchUntil)) return false;
  if (gameOpen || strengthTrainingScreen.isOpen() || battleOpen || bathUntil) return false;
  if (pet.awaitingStarter() || feedMenuUntil || confirmUntil || choiceKind || wildPromptUntil || petEventUntil) return false;
  if (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart()) return false;
  if (dexScreen.isOpen() || cardOpen || kbOpen || clockOpen || helpOpen || gameMenuOpen) return false;
  return true;
}

uint16_t lightSleepMs(uint32_t now) {
  if (!lightSleepAllowed(now)) return 0;
  uint16_t maxMs = screenOff ? 750 : (dimStage >= 2 ? 300 : 180);
  // Die Software-Schritterkennung braucht auch bei dunklem Display regelmaessige
  // Rohwerte. Kurze Light-Sleep-Intervalle erhalten sie ohne Dauer-Rendering.
  if (imuOk() && maxMs > 70) maxMs = 70;
  uint16_t ri = renderIntervalMs();
  uint32_t sinceRender = now - lastRender;
  if (sinceRender < ri) {
    uint32_t untilRender = ri - sinceRender;
    if (untilRender < maxMs) maxMs = untilRender;
  }
  if (maxMs < 40) return 0;
  return maxMs;
}

void maybeLightSleep(uint32_t now) {
  uint16_t ms = lightSleepMs(now);
  if (!ms) return;

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TP_INT, 0);
  esp_light_sleep_start();

  if (digitalRead(TP_INT) == LOW || esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    gTouchIrq = true;
  }
}

void loop() {
  uint32_t now = millis();
  uint32_t loopStart = now;
  bool petChanged = pet.update(now);
  if (petChanged && cardOpen && cardPage != 7) cardDirty = true;

  // avisa con un sonido cuando el bicho pasa a estar listo para evolucionar
  // (incluye el caso de cumplir al despertar). canEvolveNow es false durmiendo.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // aviso sombrio cuando el bicho esta a punto de escaparse por abandono
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  handleTouch();
  handleSerial();
  ensureMon();
  setPowerCacheInterval(powerSave ? 10000UL : 2000UL);
  pet.ensureDailyGoals();
  maybeOfferWildEncounter(now);
  maybeOfferPetEvent(now);
  maybeOfferMorning(now);
  maybePlayAmbientSound(now);

  uint16_t imuMs = screenOff ? 70 : (dimStage ? 60 : 40);
  imuPoll(now, imuMs);
  uint32_t stepsBefore = pet.stepsToday;
  uint32_t stepDayBefore = pet.stepDay;
  uint16_t walked = imuTakeSteps();
  // Die Erkennung filtert bereits Einzelstoesse und verlangt Geh-Rhythmus.
  // Deshalb auch direkt nach dem Abziehen von USB zaehlen: Der gecachte
  // VBUS-Status kann sonst bis zu zehn Sekunden echte Schritte verwerfen.
  if (walked) {
    pet.applyWalk(walked, rtcEpoch());
    if (cardOpen && cardPage == 7) cardDirty = true;
  }
  if ((pet.stepsToday != stepsBefore || pet.stepDay != stepDayBefore) &&
      cardOpen && cardPage == 8) cardDirty = true;
  if (imuShakeEdge() && !screenOff) {
    if (pet.isEgg()) {
      eggWiggleUntil = now + 450;
      sfxPlay(SFX_TAP);
    } else if (mainScreenReadyForShake()) {
      lastInteract = now;
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]);
      }
      if (pet.applyShake()) {
        if (audioMode() == SOUND_FULL) speciesChirpPlay(pet.speciesId, true);
        else sfxPlay(SFX_HEART);
        snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(S_HAPPY_FB));
        petEventFeedbackUntil = now + 1600;
      } else {
        sfxPlay(SFX_TAP);
      }
    }
  }

  // Die Expeditionskarte bleibt sonst als statischer Screen stehen. Ein
  // sekundenweises Dirty-Render ist nur aktiv, waehrend ihr Countdown sichtbar ist.
  static uint32_t lastExpeditionCardTick = 0;
  if (cardOpen && cardPage == 7 && now - lastExpeditionCardTick >= 1000UL) {
    lastExpeditionCardTick = now;
    cardDirty = true;
  }

  // pulsacion corta del PWR: pantalla on/off
  static uint32_t lastPwr = 0;
  if (now - lastPwr > 250) {
    lastPwr = now;
    if (pwrShortPressed()) {
      bool wasOff = screenOff;
      screenOff = !screenOff;
      if (!screenOff) {
        lastInteract = now;
        if (wasOff) {
          ignoreTouchUntil = now + 900;
          swallowGesture = true;
          wasPressed = false;
        }
      }
    }
  }

  updateBrightness(now);

  // vuelca el autoguardado periodico SOLO con la pantalla atenuada/apagada o
  // durmiendo: la escritura a NVS congela ~1s ambos cores (caché de flash off),
  // y aqui no hay animacion que se corte ni dedo esperando respuesta. Con 90s
  // de inactividad la pantalla ya atenua, asi que se vuelca enseguida; el uso
  // activo persiste igual por los guardados de cada accion (comer/jugar/...).
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }

  // anota la hora real cada 30 s (se persiste en cada save del juego)
  static uint32_t lastClock = 0;
  uint32_t clockPollMs = powerSave ? 60000UL : 30000UL;
  if (now - lastClock > clockPollMs) {
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) pet.lastSeenEpoch = e;
  }

  static bool expeditionWasActive = false;
  bool expeditionActiveNow = pet.expeditionActive(pet.lastSeenEpoch);
  if (expeditionWasActive && !expeditionActiveNow && pet.expeditionEndEpoch) {
    sfxPlay(SFX_EXPEDITION_FOUND);
    if (cardOpen && cardPage == 7) cardDirty = true;
  }
  expeditionWasActive = expeditionActiveNow;

  // latido de salud cada 5 min (para el soak test; se descarta si no hay monitor)
  static uint32_t lastHealth = 0;
  uint32_t healthMs = powerSave ? 600000UL : 300000UL;
  if (now - lastHealth > healthMs) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u rMax=%lums lMax=%lums screen=%s\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  (unsigned long)perfRenderMaxMs, (unsigned long)perfLoopMaxMs, screenName());
    perfRenderMaxMs = 0;
    perfLoopMaxMs = 0;
  }

  // Juego/combate usan intervalos conservadores para que el redibujado no pise
  // el envio DMA del frame anterior; las pantallas estaticas se saltan si no
  // estan "dirty".
  if (now - lastRender >= renderIntervalMs()) {
    lastRender = now;
    uint32_t rt0 = millis();
    render();
    perfRenderLastMs = millis() - rt0;
    if (perfRenderLastMs > perfRenderMaxMs) perfRenderMaxMs = perfRenderLastMs;
    perfRenderCount++;
  }

  maybeLightSleep(millis());
  perfLoopLastMs = millis() - loopStart;
  if (perfLoopLastMs > perfLoopMaxMs) perfLoopMaxMs = perfLoopLastMs;
  lastLoopStart = loopStart;
}

// brillo segun sueno + inactividad (proteccion del AMOLED)
void updateBrightness(uint32_t now) {
  // los eventos visibles despiertan la pantalla solos
  if (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart()) {
    lastInteract = now;
  }
  uint32_t idle = now - lastInteract;
  dimStage = (idle > 300000) ? 2 : (idle > 90000) ? 1 : 0;
  uint8_t target = pet.sleeping ? 25 : (usbPresent() ? 180 : 145);
  if (dimStage == 1) target = pet.sleeping ? 10 : 60;
  else if (dimStage == 2) target = 8;
  if (screenOff) target = 0;
  static uint8_t current = 255;
  if (target != current) {
    current = target;
    panel->setBrightness(target);
  }
}

// ---------- consola serie (provision de SD + depuracion) ----------

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;

  // Browser save backup protocol: SAVEGET -> "SAVE <bytes>" + hex data + DONE;
  // SAVEPUT <bytes> -> OK + raw data -> DONE and reboot. The Pet checksum
  // rejects altered or incomplete snapshots before NVS is changed.
  if (line == "SAVEGET") {
    uint8_t backup[Pet::BACKUP_MAX_BYTES];
    size_t size = pet.exportBackup(backup, sizeof(backup));
    if (!size) { Serial.println("ERR"); return; }
    // Normal game logs must never block when no terminal is open, hence setup
    // uses a zero TX timeout. A backup is requested by a connected browser,
    // so temporarily wait for its complete (hex) response instead of dropping it.
    Serial.setTxTimeoutMs(1500);
    Serial.printf("SAVE %u\n", (unsigned)size);
    Serial.print("SAVEHEX ");
    static const char HEX_CHARS[] = "0123456789ABCDEF";
    for (size_t i = 0; i < size; ++i) { Serial.print(HEX_CHARS[backup[i] >> 4]); Serial.print(HEX_CHARS[backup[i] & 15]); }
    Serial.println();
    Serial.println("DONE");
    Serial.flush();
    Serial.setTxTimeoutMs(0);
    return;
  }
  if (line.startsWith("SAVEPUT ")) {
    uint32_t size = (uint32_t)line.substring(8).toInt();
    if (size == 0 || size > Pet::BACKUP_MAX_BYTES) { Serial.println("ERR"); return; }
    uint8_t backup[Pet::BACKUP_MAX_BYTES];
    Serial.println("OK");
    Serial.setTimeout(6000);
    size_t got = Serial.readBytes(backup, size);
    Serial.setTimeout(1000);
    if (got != size || !pet.importBackup(backup, size)) { Serial.println("ERR"); return; }
    Serial.println("DONE");
    delay(150);
    ESP.restart();
    return;
  }

  if (line == "PERF") {
    Serial.printf("screen=%s render=%lums max=%lums count=%lu skip=%lu loop=%lums loopMax=%lums interval=%u dirty ui=%d card=%d clock=%d help=%d kb=%d menu=%d battle=%d gallery=%d\n",
                  screenName(),
                  (unsigned long)perfRenderLastMs, (unsigned long)perfRenderMaxMs,
                  (unsigned long)perfRenderCount, (unsigned long)perfRenderSkipCount,
                  (unsigned long)perfLoopLastMs, (unsigned long)perfLoopMaxMs,
                  renderIntervalMs(), uiDirty, cardDirty, clockDirty, helpDirty,
                  keyboardDirty, gameMenuDirty, battleDirty, dexScreen.isDirty());
    perfRenderMaxMs = 0;
    perfLoopMaxMs = 0;
    Serial.println("DONE");
  } else if (line == "HATCH") {
    pet.eggTap(); pet.eggTap(); pet.eggTap();
    Serial.println("DONE");
  } else if (line.startsWith("SPEC ")) {
    int n = line.substring(5).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      Serial.printf("especie #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("LVL ")) {
    pet.ageMinutes = (uint32_t)line.substring(4).toInt() * MINUTES_PER_LEVEL;
    Serial.println("DONE");
  } else if (line.startsWith("TIME ")) {
    uint32_t e = (uint32_t)line.substring(5).toInt();
    rtcSetEpoch(e);
    pet.setClock(e);
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("RTCSET ")) {  // solo RTC (simular apagados en pruebas)
    rtcSetEpoch((uint32_t)line.substring(7).toInt());
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "TIME") {
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "GAL") {
    if (dexScreen.isOpen()) {
      setScreen(nullptr);
    } else {
      dexScreen.openGrid();
      setScreen(&dexScreen);
    }
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // simula 20 tiradas de huevo (no cambia el estado del juego)
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // alterna shiny del actual (pruebas)
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // simula un dia nuevo cuidado (pruebas)
    pet.setClock(pet.lastSeenEpoch + 86400);
    pet.caress();
    Serial.printf("streak=%u bond=%u medals=0x%X\n", pet.streak, pet.bond, pet.medals);
    Serial.println("DONE");
  } else if (line == "BYE") {
    pet.startFarewell();
    Serial.println("DONE");
  } else if (line == "RUN") {
    pet.startRunaway();
    Serial.println("DONE");
  } else if (line == "BEEP") {
    sfxPlay(SFX_HATCH);  // prueba de audio
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // fuerza el estado "lista para escaparse" (test del boton)
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // borra NVS y reinicia -> partida nueva (eleccion de inicial)
    Serial.println("DONE");
    delay(100);
    ESP.restart();
  } else if (line == "REG") {
    Serial.printf("pokedex %u/%u:", pet.registeredCount(), DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isRegistered(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
#ifdef TAMAPOKE_LOCAL_TEST
  } else if (line.startsWith("TESTMON ")) {
    int split = line.indexOf(' ', 8);
    int n = line.substring(8, split < 0 ? line.length() : split).toInt();
    int shinyFlag = split < 0 ? 0 : line.substring(split + 1).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      pet.shiny = shinyFlag != 0;
      pet.sleeping = false;
      pet.ceremony = CER_NONE;
      markUiDirty();
      Serial.printf("testmon #%d %s shiny=%d\n", n, DEX_TBL[n].name, pet.shiny);
    }
    Serial.println("DONE");
  } else if (line.startsWith("TESTEVO ")) {
    int split = line.indexOf(' ', 8);
    int source = line.substring(8, split < 0 ? line.length() : split).toInt();
    int target = split < 0 ? 0 : line.substring(split + 1).toInt();
    if (source >= 1 && source <= DEX_COUNT) {
      pet.speciesId = source;
      pet.prevSpeciesId = -1;
      pet.sleeping = false;
      pet.ceremony = CER_NONE;
      pet.resetEvolutionDeferral();
      pet.careMistakes = 0;
      uint8_t requiredLevel = pet.evolutionRequiredLevelFor(target);
      pet.ageMinutes = (uint32_t)(requiredLevel > 0 ? requiredLevel - 1 : 0) * MINUTES_PER_LEVEL;
      pet.fullness = pet.joy = pet.energy = pet.hygiene = 80;
      pet.bond = 60;
      pet.trAtk = pet.trDef = pet.trSpe = 0;
      uint32_t testEpoch = 1767225600UL + 12UL * 3600UL;
      if (target == 197) testEpoch += 9UL * 3600UL;
      pet.setClock(testEpoch);
      if (source == 236) {
        if (target == 106) pet.trAtk = 80;
        else if (target == 107) pet.trDef = 80;
        else pet.trAtk = pet.trDef = 40;
      }
      markUiDirty();
      Serial.printf("testevo #%d -> #%d level=%u bond=%u\n", source, target,
                    pet.level(), pet.bond);
    }
    Serial.println("DONE");
  } else if (line == "CAUGHT") {
    Serial.printf("caught %u/%u:", pet.caughtCount(), DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isCaught(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
  } else if (line.startsWith("CAUGHT ")) {
    int n = line.substring(7).toInt();
    if (n < 1 || n > DEX_COUNT) n = pet.speciesId;
    if (n >= 1 && n <= DEX_COUNT) {
      pet.registerCaught((int16_t)n);
      Serial.printf("caught #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("BATTLE ")) {
    int split = line.indexOf(' ', 7);
    int n = line.substring(7, split < 0 ? line.length() : split).toInt();
    int8_t shinyOverride = -1;
    if (split >= 0) shinyOverride = line.substring(split + 1).toInt() ? 1 : 0;
    startBattleWith((int16_t)n, pet.level(), shinyOverride);
    Serial.printf("battle #%d %s shiny=%d caught=%d\n", battleDex,
                  (battleDex >= 1 && battleDex <= DEX_COUNT) ? DEX_TBL[battleDex].name : "?",
                  battleShiny, pet.isCaught(battleDex));
    Serial.println("DONE");
  } else if (line == "BATTLE") {
    startBattle();
    Serial.printf("battle #%d %s caught=%d\n", battleDex,
                  (battleDex >= 1 && battleDex <= DEX_COUNT) ? DEX_TBL[battleDex].name : "?",
                  pet.isCaught(battleDex));
    Serial.println("DONE");
#endif
  } else if (line == "SAVEINFO") {
    Serial.printf("fw=%s save=%s createdBoot=%d spec=%d level=%u egg=%d starter=%d age=%lu\n",
                  FW_VERSION, pet.saveLoadedFromNvs ? "loaded" : "created",
                  pet.saveCreatedThisBoot, pet.speciesId, pet.level(), pet.isEgg(),
                  pet.awaitingStarter(), (unsigned long)pet.ageMinutes);
    Serial.printf("battle wins=%u losses=%u streak=%u best=%u nick=%s\n",
                  pet.battleWins, pet.battleLosses, pet.battleStreak,
                  pet.bestBattleStreak, pet.nick);
    Serial.printf("records ball=%u catch=%u memo=%u sack=%u\n",
                  pet.gameHi, pet.catchHi, pet.memoHi, pet.strHi);
    Serial.println("DONE");
  } else if (line == "HEALTH") {
    Serial.printf("up=%lus heap=%u min=%u sd=%d mon=%d\n",
                  (unsigned long)(millis() / 1000), ESP.getFreeHeap(),
                  ESP.getMinFreeHeap(), sdReady, pmd.loaded || mon.loaded);
    Serial.println("DONE");
  } else if (line == "STATS") {
    Serial.printf("spec=%d nv=%u com=%u fel=%u ene=%u lim=%u desc=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("peso=%u fue=%u def=%u vel=%u genes=%u/%u/%u tr=%u/%u/%u baya=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe, pet.berryKnown);
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.printf("steps=%lu/%lu trail=%u shinyLuck=%u/4096 catchBonus=%u\n",
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.stepTrailRank(), pet.stepShinyChancePer4096(), pet.stepCatchBonus());
    Serial.println("DONE");
  } else if (line == "IMU") {
    Serial.printf("imu=%d addr=0x%02X mag=%.2f gyro=%.0f rawPed=%lu softPed=%lu phase=%u hour=%d\n",
                  imuOk(), imuAddr(), imuLastMagG(), imuLastGyroDps(),
                  (unsigned long)imuPedometer(), (unsigned long)imuSoftwarePedometer(),
                  currentDayPhase(), sceneHour());
    Serial.println("DONE");
  } else if (line == "SHAKE") {
    bool okShake = pet.applyShake();
    Serial.printf("shake=%d joy=%u\n", okShake, pet.joy);
    Serial.println("DONE");
  } else if (line.startsWith("WALK ")) {
    uint16_t n = (uint16_t)line.substring(5).toInt();
    uint8_t gained = pet.applyWalk(n, rtcEpoch());
    Serial.printf("walk=%u today=%lu total=%lu joy=%u bond=%u\n", gained,
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.joy, pet.bond);
    Serial.println("DONE");
  } else if (line == "STEPS") {
    Serial.printf("steps today=%lu total=%lu rank=%u shiny=%u/4096 catch+%u usb=%d imu=%d raw=%lu soft=%lu\n",
                  (unsigned long)pet.stepsToday, (unsigned long)pet.stepsTotal,
                  pet.stepTrailRank(), pet.stepShinyChancePer4096(), pet.stepCatchBonus(),
                  usbPresent(), imuOk(), (unsigned long)imuPedometer(),
                  (unsigned long)imuSoftwarePedometer());
    Serial.println("DONE");
  }
}

// ---------- entrada tactil ----------

bool inPetZone(int16_t x, int16_t y) {
  return x > 110 && x < 356 && y > 95 && y < 310;
}

// el toque se resuelve al LEVANTAR el dedo para distinguir tap de deslizar
void handleTouch() {
  static uint32_t lastPoll = 0;
  uint32_t now = millis();
  if (deadlineActive(now, ignoreTouchUntil)) {
    gTouchIrq = false;
    wasPressed = false;
    return;
  }
  if (now - lastPoll < (powerSave && screenOff ? 80UL : 20UL)) return;  // 50 Hz activo; menos si pantalla apagada
  lastPoll = millis();
  // solo tocamos el bus si el chip aviso por INT o si el dedo sigue abajo (hay
  // que detectar el levantamiento). Leer el CST9217 dormido se colgaba ~1s y
  // congelaba el loop entero; SensorLib no respeta el timeout de Wire.
  if (!gTouchIrq && !wasPressed) return;
  gTouchIrq = false;
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

  // Pantalla apagada por PWR: no despertar por cualquier roce accidental.
  // PWR corto sigue despertando al instante; touch requiere mantener pulsado.
  if (screenOff) {
    if (pressed && !wasPressed) {
      tX0 = tXl = x;
      tY0 = tYl = y;
      tStart = now;
      holdFired = false;
      wasPressed = true;
    } else if (pressed) {
      tXl = x;
      tYl = y;    
      uint32_t heldMs = millis() - tStart;
      if (!holdFired && !swallowGesture && activeScreen &&
          abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 &&
          heldMs >= activeScreen->longPressDelayMs()) {
        activeScreen->handleLongPress(tX0, tY0, heldMs);
        holdFired = true;
        return;
      }
      // pulsacion larga sin moverse sobre el bicho -> dialogo de soltar
      if (!holdFired && !swallowGesture && !activeScreen && !dexScreen.isOpen() && !cardOpen && !kbOpen && !clockOpen && !helpOpen && heldMs > 3000 &&
          abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
          !pet.isEgg() && !confirmUntil && !pet.ceremony) {
        confirmUntil = millis() + 10000;
        holdFired = true;
      }
    } else {
      wasPressed = false;
      holdFired = false;
    }
    return;
  }

  // El entrenamiento cuenta cada pulsacion inmediatamente.
  if (activeScreen == &strengthTrainingScreen) {
    if (pressed && !wasPressed) {
      activeScreen->handlePress(y);
    }
    wasPressed = pressed;
    return;
  }

  if (pressed && !wasPressed) {  // empieza el gesto
    tX0 = tXl = x;
    tY0 = tYl = y;
    tStart = millis();
    holdFired = false;
    swallowGesture = (dimStage > 0) || screenOff;  // si estaba a oscuras, solo despierta
    screenOff = false;
    lastInteract = millis();
    if (!swallowGesture && gameOpen && (gameMode == 0 || gameMode == 1)) {
      if (activeScreen) activeScreen->handleTap(x, y);  // cuenta al tocar, no al soltar
      swallowGesture = true;  // evita doble tap al levantar el dedo
    }
  } else if (pressed) {  // sigue apoyado
    tXl = x;
    tYl = y;
    // pulsacion larga sin moverse sobre el bicho -> dialogo de soltar
    if (!holdFired && !swallowGesture && !activeScreen && !dexScreen.isOpen() && !cardOpen && !kbOpen && !clockOpen && !helpOpen && millis() - tStart > 3000 &&
        abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
        !pet.isEgg() && !confirmUntil && !pet.ceremony) {
      confirmUntil = millis() + 10000;
      holdFired = true;
    }
  } else if (wasPressed) {  // levanta el dedo: resolver gesto
    lastInteract = millis();
    int dx = tXl - tX0, dy = tYl - tY0;
    uint32_t dt = millis() - tStart;
    if (!holdFired && !swallowGesture) {
      if (abs(dx) > 80 && abs(dy) < 70 && dt < 800) onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && dt < 800) onSwipeV(dy > 0 ? 1 : -1);
      else if (dt < 1500 && abs(dx) < 40 && abs(dy) < 40) onTap(tX0, tY0);
    }
  }
  wasPressed = pressed;
}

// deslizar vertical: abre/cierra la ficha del bicho
void openClock();  // prototipo
int sceneHour();
uint8_t currentDayPhase();
StrId dayPhaseTextId(uint8_t phase);
bool mainScreenReadyForShake();
void maybeOfferMorning(uint32_t now);
uint32_t pmdActTotalMs(const PmdAct &a);
void startCleanGame();
void startTypeGame();
uint32_t expeditionNowEpoch();
bool expeditionHudVisible();
bool inExpeditionHudHit(int16_t x, int16_t y);
void openExpeditionCard();
void drawExpeditionHud();
bool inStepsHudHit(int16_t x, int16_t y);
void openStepsCard();
void drawStepsHud();

void onSwipeV(int dir) {
  if (helpOpen) { activeScreen->handleSwipeV(dir); return; }
  if (pet.awaitingStarter()) return;  // bloqueado durante la eleccion de inicial
  if (wildPromptUntil && deadlineActive(millis(), wildPromptUntil)) return;
  if (wildPromptUntil) wildPromptUntil = 0;
  if (gameMenuOpen) return;
  if (gameOpen || dexScreen.isOpen() || kbOpen || strengthTrainingScreen.isOpen() || battleOpen || pet.ceremony) return;
  if (clockOpen) { clockOpen = false; setScreen(nullptr); sfxPlay(SFX_TAP); return; }
  if (cardOpen) {
    if (activeScreen) activeScreen->handleSwipeV(dir);
    return;
  }
  if (activeScreen) {
    activeScreen->handleSwipeV(dir);
    return;
  }

  if (dir > 0) {                    // deslizar abajo: ajustar hora
    if (!confirmUntil && !feedMenuUntil) openClock();
  } else if (!pet.isEgg() && !confirmUntil && !feedMenuUntil) {
    cardOpen = true;                // deslizar arriba: ficha
    cardPage = 0;
    cardDirty = true;
    setScreen(petCardScreenForPage(cardPage));
    sfxPlay(SFX_MENU);
  }
}

// deslizar: dir +1 = hacia la derecha
void onSwipe(int dir) {
  if (helpOpen) {
    activeScreen->handleSwipe(dir);
    return;
  }
  if (pet.awaitingStarter()) return;  // bloqueado durante la eleccion de inicial
  if (wildPromptUntil && deadlineActive(millis(), wildPromptUntil)) return;
  if (wildPromptUntil) wildPromptUntil = 0;
  if (gameMenuOpen) return;
  if (gameOpen || kbOpen || clockOpen || battleOpen) return;
  if (cardOpen) {
    if (activeScreen) activeScreen->handleSwipe(dir);
    return;
  }

  // Telas modulares tratam seus próprios gestos horizontais.
  if (activeScreen) {
    activeScreen->handleSwipe(dir);
    return;
  }
  // 2. Na tela principal (nem Pokédex nem Party abertos)
  if (!dexScreen.isOpen()) {
    if (!pet.ceremony && !confirmUntil) {
      if (dir < 0) {
        // Esquerda -> Abre a Pokédex
        dexScreen.openGrid();
        setScreen(&dexScreen);
        sfxPlay(SFX_MENU);
      } else if (!pet.isEgg()) {
        // Direita -> Abre a tela modular da Party
        setScreen(&partyScreen);
        sfxPlay(SFX_MENU);
      }
    }
    return;
  }
}

struct GameMenuTile {
  int16_t x, y, w, h;
  uint8_t game;
};

static const GameMenuTile GAME_MENU_TILES[5] = {
  { 88, 156, 138, 58, 0 },
  { 240, 156, 138, 58, 1 },
  { 88, 226, 138, 58, 2 },
  { 240, 226, 138, 58, 3 },
  { 94, 296, 278, 62, 4 },
};

int8_t gameMenuHit(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 5; i++) {
    const GameMenuTile &t = GAME_MENU_TILES[i];
    if (x >= t.x && x <= t.x + t.w && y >= t.y && y <= t.y + t.h) return i;
  }
  return -1;
}

void startGameMenuChoice(uint8_t idx) {
  switch (GAME_MENU_TILES[idx].game) {
    case 0: startGame(); break;
    case 1: startCatchGame(); break;
    case 2: startMemoGame(); break;
    case 3: startCleanGame(); break;
    case 4: startTypeGame(); break;
  }
}

static bool isMinigameScreen(const Screen *screen) {
  return screen == &gameBallScreen || screen == &gameCatchScreen ||
         screen == &gameMemoScreen || screen == &gameCleanScreen ||
         screen == &gameTypeScreen;
}

void onTap(int16_t x, int16_t y) {
  // Serial.printf("TOUCH %d %d\n", x, y);  // diagnostico (silenciado: satura el log)
  if (activeScreen) {
    Screen *tappedScreen = activeScreen;
    activeScreen->handleTap(x, y);
    if (activeScreen == tappedScreen && isMinigameScreen(tappedScreen) && !gameOpen)
      setScreen(nullptr);
    return;
  }
  if (pet.ceremony) return;  // durante la despedida no hay botones
  if (petEventUntil && deadlineActive(millis(), petEventUntil) && inPetEventHit(x, y)) {
    acceptPetEvent();
    return;
  }
  if (gameMenuOpen) {
    int8_t hit = gameMenuHit(x, y);
    if (hit >= 0) { lockTouchBrief(140); startGameMenuChoice((uint8_t)hit); }
    else { gameMenuOpen = false; markUiDirty(); lockTouchBrief(); sfxPlay(SFX_TAP); }
    return;
  }
  if (wildPromptUntil) {
    if (deadlineActive(millis(), wildPromptUntil)) {
      bool fight = (x >= 93 && x <= 373 && y >= 226 && y <= 270);
      bool later = (x >= 93 && x <= 373 && y >= 278 && y <= 322);
      if (fight) {
        startBattleWith(wildPromptDex, wildPromptLevel, wildPromptShiny ? 1 : 0);
      } else if (later) {
        wildPromptUntil = 0;
        scheduleNextWild(millis());
        sfxPlay(SFX_TAP);
      }
    } else {
      wildPromptUntil = 0;
    }
    return;
  }
  if (inExpeditionHudHit(x, y)) {
    openExpeditionCard();
    return;
  }
  if (inStepsHudHit(x, y)) {
    openStepsCard();
    return;
  }
  if (choiceKind) {          // dialogo de decision o selector de objetivo evolutivo
    if (choiceKind == 3) {
      uint8_t options = pet.evolutionOptionCount();
      for (uint8_t i = 0; i < options; i++) {
        int y = 158 + i * 45;
        if (x >= 82 && x <= 394 && y <= 194 + i * 45 && y >= 158 + i * 45) {
          int16_t target = pet.evolutionOption(i);
          int16_t old = pet.speciesId;
          pet.evolveTo(target);
          evoPmd.load(old, pet.shiny);
          choiceKind = 0;
          return;
        }
      }
      if (y >= 392 && y <= 444) choiceKind = 0;
      return;
    }
    bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);  // accion
    bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);  // mantener / quedaros
    if (choiceKind == 1) {                 // evolucion
      if (b1) {
        if (pet.evolutionOptionCount() > 1) {
          choiceKind = 3;
          choiceUntil = millis() + 12000;
          return;
        }
        int16_t old = pet.speciesId;
        pet.evolveTo(pet.evolutionOption(0));
        evoPmd.load(old, pet.shiny);
      }
      else if (b2) pet.declineEvolve();
    } else if (choiceKind == 2) {          // despedida
      if (b1) pet.startFarewell();
      else if (b2) pet.declineFarewell();
    }
    choiceKind = 0;
    return;
  }
  if (confirmUntil) {        // dialogo "soltar?": SI / NO
    if (deadlineActive(millis(), confirmUntil) && x >= 118 && x <= 218 && y >= 252 && y <= 304) {
      pet.release();
    }
    confirmUntil = 0;
    return;
  }
  if (feedMenuUntil) {       // selector de comida
    if (deadlineActive(millis(), feedMenuUntil) && y >= 288 && y <= 352 && x >= 101 && x <= 365) {
      int item = (x - 101) / 66;
      if (item == 3) pet.feedCandy();
      else pet.feedBerry(item);
      sfxPlay(SFX_EAT);
    }
    feedMenuUntil = 0;
    return;
  }
  if (pet.isEgg()) {
    pet.eggTap();
    sfxPlay(SFX_TAP);
    return;
  }
  // boton de evolucion: abre el dialogo evolucionar/mantener
  if (pet.wantEvolveButton() && x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    if (pet.canEvolveNow()) {
      choiceKind = 1; choiceUntil = millis() + 12000;
    } else {
      StrId blocked = pet.evolutionNeedsMoreBond() ? S_EVO_NEED_BOND : S_EVO_BLOCKED;
      snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(blocked));
      petEventFeedbackUntil = millis() + 1800;
      sfxPlay(SFX_DENY);
    }
    return;
  }
  // botones de final (mismo recuadro): escapada directa; despedida abre dialogo
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { pet.startRunaway(); return; }
    if (pet.wantFarewellButton()) { choiceKind = 2; choiceUntil = millis() + 12000; return; }
  }
  for (int i = 0; i < 4; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT) {
      Serial.printf("BTN %d\n", i);
      sfxPlay(SFX_TAP);
      if (i == 0) {
        if (!pet.sleeping) {
          feedMenuUntil = millis() + 6000;
          sfxPlay(SFX_MENU);
        }
      } else if (i == 1) {
        if (!pet.sleeping) {
          gameMenuOpen = true;
          gameMenuDirty = true;
          lockTouchBrief();
          sfxPlay(SFX_MENU);
        }
      } else if (i == 2) {
        pet.toggleLight();
      } else {
        startBath();
      }
      return;
    }
  }
  // tocar al bicho = caricia
  if (inPetZone(x, y)) {
    Serial.println("PET");
    uint8_t r = pet.interactPet(currentDayPhase() == 2);
    // Der Spezies-Chirp ist direkte Rueckmeldung fuer jeden Pet-Tap. Er darf
    // nicht von der zehnminuetigen Pflege-Belohnung abhaengen.
    if (!pet.sleeping && audioMode() == SOUND_FULL) speciesChirpPlay(pet.speciesId, true);
    if (r == PET_INTERACT_NONE) return;
    StrId msg = S_WAIT;
    if (r & PET_INTERACT_BOND) msg = S_BOND_GAIN;
    else if (r & PET_INTERACT_JOY) msg = S_HAPPY_FB;
    snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(msg));
    petEventFeedbackUntil = millis() + 1600;
    if (!pet.sleeping && audioMode() != SOUND_FULL)
      sfxPlay((r & PET_INTERACT_BOND) ? SFX_HEART : SFX_TAP);
  }
}

// ---------- render ----------

bool gNight = false;  // noche real (por hora) o durmiendo: lo fija render()
uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// ---------- escena de fondo: bioma del tipo + hora real del RTC ----------

#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define HORIZON 232  // linea donde el cielo se encuentra con el suelo

uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

static uint8_t watermarkSlot() {
  return (uint8_t)((millis() / 20000UL) & 3U);
}

bool watermarkNeedsRedraw() {
  return !screenOff && watermarkDrawnSlot != (int8_t)watermarkSlot();
}

static uint16_t watermarkPixel(uint16_t base) {
  int r = ((base >> 11) & 31) * 255 / 31;
  int g = ((base >> 5) & 63) * 255 / 63;
  int b = (base & 31) * 255 / 31;
  int luminance = (r * 30 + g * 59 + b * 11) / 100;
  return lerp565(base, luminance > 145 ? 0x0000 : 0xFFFF, 1, 4);
}

static void drawWatermark(uint8_t slot) {
  static const uint8_t GLYPHS[3][7] = {
    { 0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E },  // @
    { 0x1F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E },  // S
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F },  // E
  };
  static const int16_t POS[4][2] = {
    { 74, 74 }, { 374, 74 }, { 74, 384 }, { 374, 384 },
  };
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int16_t ox = POS[slot][0];
  int16_t oy = POS[slot][1];
  for (uint8_t glyph = 0; glyph < 3; glyph++) {
    for (uint8_t row = 0; row < 7; row++) {
      uint8_t bits = GLYPHS[glyph][row];
      for (uint8_t col = 0; col < 5; col++) {
        if (!(bits & (1U << (4 - col)))) continue;
        int16_t x = ox + glyph * 6 + col;
        int16_t y = oy + row;
        if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) continue;
        uint32_t idx = (uint32_t)y * LCD_WIDTH + x;
        fb[idx] = watermarkPixel(fb[idx]);
      }
    }
  }
}

void flushStampedFrame() {
  uint8_t slot = watermarkSlot();
  drawWatermark(slot);
  watermarkDrawnSlot = (int8_t)slot;
  gfx->flush();
}

int sceneHour() {
  return sceneHourFromEpoch(pet.lastSeenEpoch);
}

uint8_t currentDayPhase() {
  return dayPhaseFromHour(sceneHour());
}

StrId dayPhaseTextId(uint8_t phase) {
  switch (phase) {
    case 0: return S_MORNING;
    case 2: return S_EVENING;
    case 3: return S_NIGHT;
    default: return S_DAY;
  }
}

// suelo de cada bioma de dia (de noche se mezcla hacia el azul nocturno)
static const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 pradera
  C565(0xdc, 0xca, 0x94),  // 1 playa (arena)
  C565(0x4f, 0x8a, 0x55),  // 2 bosque
  C565(0x8a, 0x55, 0x44),  // 3 volcan
  C565(0xa8, 0x90, 0x6a),  // 4 montana
  C565(0xe6, 0xee, 0xf5),  // 5 nieve
};

void drawClouds(uint32_t now, uint16_t col) {
  for (int k = 0; k < 2; k++) {
    int cx = (int)((now / 50 + k * 250) % 560) - 40;
    int cy = 70 + k * 34;
    gfx->fillCircle(cx, cy, 16, col);
    gfx->fillCircle(cx + 18, cy + 3, 13, col);
    gfx->fillCircle(cx - 15, cy + 4, 12, col);
  }
}

void drawScene(uint8_t biome, uint32_t now, bool night) {
  int h = sceneHour();
  uint16_t top, bot;
  if (night)            { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (h < 8)       { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // amanecer
  else if (h < 18)      { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // dia
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // atardecer

  // cielo en bandas
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // sol o luna
  if (night) {
    gfx->fillCircle(360, 78, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 72, 22, lerp565(top, bot, 78, HORIZON));  // creciente
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (h < 18) {
    gfx->fillCircle(360, 84, 26, h < 8 ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // sol poniente
  }

  // mar de la playa: una franja de agua sobre la arena
  uint16_t soil = BIOME_SOIL[biome < 6 ? biome : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (biome == 1) {
    uint16_t sea = night ? C565(0x1c, 0x34, 0x52) : C565(0x4f, 0x96, 0xc4);
    gfx->fillRect(0, HORIZON - 26, 466, 26, sea);
    for (int i = 0; i < 3; i++) {
      int wy = HORIZON - 22 + i * 7;
      uint16_t fc = night ? C565(0x3a, 0x58, 0x78) : C565(0xbf, 0xe6, 0xf5);
      gfx->fillRect(60 + ((now / 60 + i * 30) % 60), wy, 26, 2, fc);
      gfx->fillRect(300 - ((now / 60 + i * 20) % 60), wy, 26, 2, fc);
    }
  }

  // suelo
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // detalles del bioma
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // bosque: coniferas en silueta
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // volcan: rocas y brasas
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // montana: cumbres al fondo
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // nieve: copos cayendo
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // pradera: matas de hierba
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

void render() {
  if (watermarkNeedsRedraw()) {
    cardDirty = true;
    clockDirty = true;
    helpDirty = true;
    keyboardDirty = true;
    gameMenuDirty = true;
    battleDirty = true;
    if (activeScreen) activeScreen->markDirty();
  }
  if (staticScreenClean()) { perfRenderSkipCount++; return; }
  if (activeScreen) {
    Screen *renderingScreen = activeScreen;
    renderingScreen->render();
    if (activeScreen == renderingScreen && isMinigameScreen(renderingScreen) && !gameOpen)
      setScreen(nullptr);
    return;
  }
  int h = sceneHour();
  gNight = isVisualNight(h, pet.sleeping);
  // drawScene cubre los 466x466 completos: sin fillScreen(NEGRO) previo para
  // que un flush DMA solapado nunca capture negro a medias (anti-parpadeo)
  drawScene(pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome, millis(), gNight);

  if (pet.ceremony) {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    const char *msg = (pet.ceremony == CER_FAREWELL) ? T(S_FAREWELL)
                      : (pet.ceremony == CER_RUNAWAY) ? T(S_RUNAWAY)
                                                      : T(S_GOODBYE);
    drawHeader(dexName(pet.speciesId), d.accent, msg);
    drawCeremony();
    flushStampedFrame();
    return;
  }

  if (pet.isEgg()) {
    drawHeader(T(S_EGG_HDR), inkColor(), eggMsg());
    int s = 5, x = CX - 16 * s, y = PET_CY - 16 * s;
    if (deadlineActive(millis(), eggWiggleUntil))
      x += ((millis() / 40) % 2) ? 6 : -6;
    drawMap(SPR_EGG, SPRITE_H, x, y, s, false);
    if (pet.eggCracks() >= 1)
      for (auto &c : CRACK1) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggCracks() >= 2)
      for (auto &c : CRACK2) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggRarity() >= R_RARO) {
      const char *rar = (pet.eggRarity() == R_LEGENDARIO) ? T(S_EGG_LEGEND) : T(S_EGG_RARE);
      gfx->setTextColor(pet.eggRarity() == R_LEGENDARIO ? UI_BAR_WARN : 0x4C98);
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(rar) * 6, 316);
      gfx->print(rar);
    }
    char reg[24];
    snprintf(reg, sizeof(reg), T(S_POKEDEX_FMT), pet.registeredCount(), DEX_COUNT);
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    gfx->setTextColor(inkColor());
    gfx->setTextSize(2);
    gfx->setCursor(CX - strlen(reg) * 6, 348);
    gfx->print(reg);
    drawStepsHud();
  } else {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    char name[28];
    const char *base = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
    snprintf(name, sizeof(name), T(S_NAME_FMT), pet.shiny ? "*" : "", base, pet.level());
    drawHeader(name, gNight ? UI_INK_NIGHT : d.accent, statusMsg());
    drawStreakBadge();
    drawStepsHud();
    drawPet();
    drawBath();
    drawPoops();
    drawExpeditionHud();
    // panel inferior: base limpia para barras y botones sobre el paisaje
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    drawBars();
    drawButtons();
    drawCelebration();
    drawPetEvent();
    if (gameMenuOpen) drawGameMenu();
    if (pet.wantEvolveButton()) drawEvolveButton();        // CTA rojo: evolucionar
    else if (pet.canRunawayNow()) drawRunawayButton();     // CTA sombrio: escapada (abandono)
    else if (pet.wantFarewellButton()) drawFarewellButton();  // CTA dorado: despedida
  }

  if (pet.sleeping) {
    gfx->setTextColor(UI_INK_NIGHT);
    gfx->setTextSize(3);
    gfx->setCursor(320, 130);
    gfx->print("Zz");
  }

  if (morningUntil) {
    if (deadlineReached(millis(), morningUntil)) morningUntil = 0;
    else {
      const char *hello = T(S_GOOD_MORNING);
      gfx->fillRoundRect(90, 78, 286, 44, 14, UI_WHITE);
      gfx->drawRoundRect(90, 78, 286, 44, 14, inkColor());
      gfx->setTextColor(inkColor());
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(hello) * 6, 92);
      gfx->print(hello);
    }
  }

  if (wildPromptUntil) {
    if (deadlineReached(millis(), wildPromptUntil)) wildPromptUntil = 0;
    else drawWildPrompt();
  }

  // selector de comida
  if (feedMenuUntil) {
    if (deadlineReached(millis(), feedMenuUntil)) {
      feedMenuUntil = 0;
    } else {
      gfx->fillRoundRect(101, 288, 264, 64, 14, UI_WHITE);
      gfx->drawRoundRect(101, 288, 264, 64, 14, inkColor());
      drawMap(SPR_ICON_FOOD, 16, 110, 296, 3, false);
      drawMap(SPR_ICON_BERRY_B, 16, 176, 296, 3, false);
      drawMap(SPR_ICON_BERRY_G, 16, 242, 296, 3, false);
      drawMap(SPR_ICON_CANDY, 16, 308, 296, 3, false);
    }
  }

  // dialogo "soltar?" (pulsacion larga sobre el bicho)
  if (confirmUntil) {
    if (deadlineReached(millis(), confirmUntil)) {
      confirmUntil = 0;
    } else {
      gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
      gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
      char q[28];
      snprintf(q, sizeof(q), T(S_RELEASE_FMT), dexName(pet.speciesId));
      gfx->setTextColor(UI_INK);
      gfx->setTextSize(2);
      gfx->setCursor(CX - strlen(q) * 6, 196);
      gfx->print(q);
      gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      gfx->setCursor(118 + (100 - (int)strlen(T(S_YES)) * 12) / 2, 270);
      gfx->print(T(S_YES));
      gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
      gfx->setCursor(248 + (100 - (int)strlen(T(S_NO)) * 12) / 2, 270);
      gfx->print(T(S_NO));
    }
  }

  // dialogo de decision (evolucionar/mantener, despedirse/quedaros)
  if (choiceKind) {
    if (deadlineReached(millis(), choiceUntil)) choiceKind = 0;
    else drawChoiceDialog();
  }

  flushStampedFrame();
}

// ---------- minijuego: toques con la pokeball ----------

void drawGameMenu() {
  gameMenuDirty = false;
  gfx->fillRoundRect(78, 112, 310, 266, 18, UI_WHITE);
  gfx->drawRoundRect(78, 112, 310, 266, 18, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  const char *title = T(S_PLAY);
  gfx->setCursor(CX - strlen(title) * 9, 124);
  gfx->print(title);
  const char *labels[5] = { T(S_GAME_BALL), T(S_GAME_CATCH), T(S_GAME_MEMO), T(S_GAME_CLEAN), T(S_GAME_TYPE) };
  uint16_t cols[5] = { UI_BAR_BAD, UI_BAR_WARN, 0x4C98, UI_BAR_OK, 0xF3B7 };
  for (int i = 0; i < 5; i++) {
    const GameMenuTile &t = GAME_MENU_TILES[i];
    gfx->fillRoundRect(t.x, t.y, t.w, t.h, 14, cols[i]);
    gfx->drawRoundRect(t.x, t.y, t.w, t.h, 14, UI_INK);
    gfx->setTextColor(i == 1 ? UI_INK : UI_BG_DAY);
    gfx->setTextSize(2);
    gfx->setCursor(t.x + (t.w - (int)strlen(labels[i]) * 12) / 2, t.y + (t.h - 16) / 2);
    gfx->print(labels[i]);
  }
}

void startGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  gameOpen = true;
  gameMode = 0;
  gameOverUntil = 0;
  gameScore = 0;
  gameMisses = 0;
  gameNewHi = false;
  gameGain = 0;
  hitTime = 0;
  ballLastHitAt = 0;
  gamePetX = 233;
  respawnBall();
  setScreen(&gameBallScreen);
  sfxPlay(SFX_GAME_START);
}

void spawnCatchTarget() {
  catchX = 86 + random(294);
  catchY = 118 + random(206);
  catchIcon = random(3);
  uint32_t life = 980;
  uint32_t speedup = (uint32_t)gameScore * 35;
  if (speedup > 530) speedup = 530;
  life -= speedup;
  catchTargetUntil = millis() + life;
}

void startCatchGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  gameOpen = true;
  gameMode = 1;
  gameOverUntil = 0;
  gameScore = 0;
  gameMisses = 0;
  gameNewHi = false;
  gameGain = 0;
  catchUntil = millis() + 20000;
  spawnCatchTarget();
  setScreen(&gameCatchScreen);
  sfxPlay(SFX_GAME_START);
}

void startMemoRound() {
  if (memoLen < 14) memoSeq[memoLen++] = random(4);
  memoShow = 0;
  memoInput = 0;
  memoActivePad = -1;
  memoHintPad = -1;
  memoShowing = true;
  memoNextAt = millis() + 350;
  memoTurnUntil = 0;
}

void startMemoGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  gameOpen = true;
  gameMode = 2;
  gameOverUntil = 0;
  gameScore = 0;
  gameNewHi = false;
  gameGain = 0;
  memoLen = 0;
  memoRounds = 0;
  memoFlashPad = -1;
  memoHintPad = -1;
  memoFlashUntil = memoFailUntil = memoTurnUntil = 0;
  startMemoRound();
  setScreen(&gameMemoScreen);
  sfxPlay(SFX_GAME_START);
}

void spawnCleanSpot() {
  for (uint8_t i = 0; i < 4; i++) {
    if (cleanAlive[i]) continue;
    cleanX[i] = 88 + random(290);
    cleanY[i] = 122 + random(224);
    cleanAlive[i] = true;
    cleanActive++;
    return;
  }
}

void startCleanGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  gameOpen = true;
  gameMode = 3;
  gameOverUntil = 0;
  gameScore = 0;
  gameMisses = 0;
  gameNewHi = false;
  gameGain = 0;
  cleanActive = 0;
  for (uint8_t i = 0; i < 4; i++) cleanAlive[i] = false;
  cleanUntil = millis() + 18000;
  cleanSpawnAt = millis();
  spawnCleanSpot();
  setScreen(&gameCleanScreen);
  sfxPlay(SFX_GAME_START);
}

static const uint8_t TYPE_DEF_POOL[] = {
  TYPE_GRASS, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_ROCK, TYPE_GROUND,
  TYPE_FLYING, TYPE_POISON, TYPE_PSYCHIC, TYPE_GHOST, TYPE_DRAGON, TYPE_ICE
};
static const uint8_t TYPE_COUNTER_POOL[] = {
  TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_GROUND, TYPE_WATER, TYPE_WATER,
  TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_BUG, TYPE_GHOST, TYPE_ICE, TYPE_FIRE
};
static const uint8_t TYPE_OPTION_POOL[] = {
  TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC, TYPE_GRASS, TYPE_ICE,
  TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_FLYING, TYPE_PSYCHIC, TYPE_BUG,
  TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON
};

void nextTypeQuestion() {
  uint8_t q = (uint8_t)random(sizeof(TYPE_DEF_POOL));
  typeEnemy = TYPE_DEF_POOL[q];
  uint8_t correct = TYPE_COUNTER_POOL[q];
  typeCorrect = (uint8_t)random(3);
  for (uint8_t i = 0; i < 3; i++) typeChoice[i] = TYPE_NONE;
  typeChoice[typeCorrect] = correct;
  for (uint8_t i = 0; i < 3; i++) {
    if (i == typeCorrect) continue;
    uint8_t cand;
    do {
      cand = TYPE_OPTION_POOL[random(sizeof(TYPE_OPTION_POOL))];
    } while (cand == correct || cand == typeChoice[0] || cand == typeChoice[1] || cand == typeChoice[2] ||
             battleTypeEffectPct(cand, typeEnemy, TYPE_NONE) > 100);
    typeChoice[i] = cand;
  }
  typeUntil = millis() + 4200;
}

void startTypeGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameMenuOpen = false;
  gameOpen = true;
  gameMode = 4;
  gameOverUntil = 0;
  gameScore = 0;
  gameMisses = 0;
  gameNewHi = false;
  gameGain = 0;
  nextTypeQuestion();
  setScreen(&gameTypeScreen);
  sfxPlay(SFX_GAME_START);
}

void respawnBall() {
  ballX = 112 + random(242);
  ballY = 82;
  float sp = 3.35f + gameScore * 0.14f;
  if (sp > 8.4f) sp = 8.4f;
  ballVX = random(2) ? sp : -sp;
  ballVX += ((int)random(9) - 4) * 0.28f;
  ballVY = 2.05f;
}

int ballHitRadius() {
  if (gameScore >= 20) return 38;
  if (gameScore >= 8) return 44;
  return 50;
}

void finishCatchGame() {
  gameNewHi = (gameScore > pet.catchHi);
  gameGain = pet.applyCatchResult(gameScore);
  sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  gameOverUntil = millis() + 4000;
}

void finishMemoGame() {
  gameScore = memoRounds;
  gameNewHi = (memoRounds > pet.memoHi);
  gameGain = pet.applyMemoResult(memoRounds);
  sfxPlay(gameNewHi && memoRounds > 0 ? SFX_MEDAL : SFX_LEVEL);
  gameOverUntil = millis() + 4000;
}

void memoPadSound(uint8_t pad) {
  if (pad < 4) sfxPlay((uint8_t)(SFX_MEMO_PAD_0 + pad));
}

void finishCleanGame() {
  gameNewHi = (gameScore > pet.cleanHi);
  gameGain = pet.applyCleanResult(gameScore);
  sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  gameOverUntil = millis() + 4000;
}

void finishTypeGame() {
  gameNewHi = (gameScore > pet.typeHi);
  gameGain = pet.applyTypeResult(gameScore);
  sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  gameOverUntil = millis() + 4000;
}

void stepGame() {
  float grav = 1.14f + gameScore * 0.046f;
  if (gameScore >= 5) grav += 0.14f;
  if (gameScore >= 12) grav += 0.18f;
  if (grav > 2.10f) grav = 2.10f;
  ballVX += sinf((millis() + gameScore * 97) * 0.018f) * 0.11f;
  if (random(100) < 7) ballVX += ((int)random(7) - 3) * 0.22f;
  ballVY += grav;
  ballX += ballVX;
  ballY += ballVY;
  // rebote en la pared circular
  float dx = ballX - CX, dy = ballY - CY;
  float d = sqrtf(dx * dx + dy * dy);
  if (d > 205) {
    float nx = dx / d, ny = dy / d;
    float dot = ballVX * nx + ballVY * ny;
    if (dot > 0) {
      ballVX = (ballVX - 2 * dot * nx) * 1.05f;
      ballVY = (ballVY - 2 * dot * ny) * 0.88f;
      ballVX += ((int)random(9) - 4) * 0.18f;
      sfxPlay(SFX_BALL_BOUNCE);
    }
    ballX = CX + nx * 205;
    ballY = CY + ny * 205;
  }
  if (ballY > 384) {  // al suelo
    if (++gameMisses >= 3) {
      gameNewHi = (gameScore > pet.gameHi);
      pet.playResult(gameScore);  // actualiza el record y da felicidad
      sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
      gameOverUntil = millis() + 4000;
    } else {
      respawnBall();
      sfxPlay(SFX_BALL_MISS);
    }
  }
  // el bicho la sigue por abajo
  float chase = (ballX - gamePetX) * 0.12f;
  if (chase > 7) chase = 7;
  if (chase < -7) chase = -7;
  gamePetX += chase;
}

void drawGameScene();  // prototipo (definida mas abajo)
const char *battleTypeName(uint8_t type);
uint16_t battleTypeColor(uint8_t type);

// fondo del minijuego: hatibat del bicho (cielo por hora + suelo del bioma)
void drawGameScene() {
  int hh = sceneHour();
  bool night = hh < 6 || hh >= 20;
  uint16_t top, bot;
  if (night)       { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (hh < 8) { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }
  else if (hh < 18){ top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }
  else             { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }
  int hor = 376;
  for (int y = 0; y < hor; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, hor));
  if (night)
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  uint8_t bio = pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  gfx->fillRect(0, hor, 466, 466 - hor, soil);
}

void stepMemoGame() {
  uint32_t now = millis();
  if (memoFailUntil) {
    if (deadlineReached(now, memoFailUntil)) {
      memoFailUntil = 0;
      memoHintPad = -1;
      finishMemoGame();
    }
    return;
  }
  if (!memoShowing || deadlineActive(now, memoNextAt)) return;
  if (memoActivePad >= 0) {
    memoActivePad = -1;
    memoShow++;
    if (memoShow >= memoLen) {
      memoShowing = false;
      memoInput = 0;
      memoTurnUntil = now + 520;
    } else {
      memoNextAt = now + 150;
    }
    return;
  }
  memoActivePad = memoSeq[memoShow];
  memoPadSound((uint8_t)memoActivePad);
  memoNextAt = now + 480;
}

// ---------- combate salvaje manual ----------

uint16_t battleHpFor(const BattleStats &stats) {
  return stats.hp ? stats.hp : (uint16_t)(30 + stats.level * 5 + stats.def);
}

BattleStats petBattleStats() {
  BattleStats stats = {};
  stats.level = pet.level();
  stats.atk = pet.atkStat();
  stats.def = pet.defStat();
  stats.spe = pet.speStat();
  stats.hp = 0;
  if (!pet.isEgg() && pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    stats.type1 = DEX_TBL[pet.speciesId].type1;
    stats.type2 = DEX_TBL[pet.speciesId].type2;
  }
  return stats;
}

void scheduleNextWild(uint32_t now) {
  nextWildEligible = now + WILD_COOLDOWN_MS;
}

bool mainScreenReadyForWild() {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (battleOpen || gameOpen || gameMenuOpen || strengthTrainingScreen.isOpen() || cardOpen || dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || petEventUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}

void maybeOfferWildEncounter(uint32_t now) {
  if (nextWildEligible == 0) {
    scheduleNextWild(now);
    return;
  }
  if (wildPromptUntil) return;
  if (now - lastWildCheck < WILD_CHECK_MS) return;
  lastWildCheck = now;
  if (deadlineActive(now, nextWildEligible)) return;
  if (!mainScreenReadyForWild()) return;
  int16_t cand = pickWildSpecies((uint8_t)random(100), currentDayPhase());
  uint8_t phase = currentDayPhase();
  uint8_t chance = (phase == 3) ? 4 : (phase == 0 ? 7 : 8);
  if (phase == 3 && cand >= 1 && cand <= DEX_COUNT) {
    const DexEntry &d = DEX_TBL[cand];
    if (d.type1 == TYPE_GHOST || d.type2 == TYPE_GHOST ||
        d.type1 == TYPE_POISON || d.type2 == TYPE_POISON) chance = 8;
  }
  if ((uint8_t)random(100) >= chance) return;

  wildPromptDex = cand;
  wildPromptLevel = wildLevelFor(pet.level(), (uint8_t)random(100));
  wildPromptShiny = (random(4096) < pet.stepShinyChancePer4096());
  wildPromptUntil = now + WILD_PROMPT_MS;
  scheduleNextWild(now);
  sfxPlay(SFX_MENU);
}

void scheduleNextPetEvent(uint32_t now) {
  nextPetEventEligible = now + PET_EVENT_COOLDOWN_MS;
}

bool mainScreenReadyForPetEvent() {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (battleOpen || gameOpen || gameMenuOpen || strengthTrainingScreen.isOpen() || cardOpen || dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil) return false;
  if (pet.evolving() || pet.wantEvolveButton() || pet.canRunawayNow() || pet.wantFarewellButton()) return false;
  return true;
}

void maybeOfferPetEvent(uint32_t now) {
  if (nextPetEventEligible == 0) {
    scheduleNextPetEvent(now);
    return;
  }
  if (petEventUntil) {
    if (deadlineReached(now, petEventUntil)) petEventUntil = 0;
    return;
  }
  if (now - lastPetEventCheck < PET_EVENT_CHECK_MS) return;
  lastPetEventCheck = now;
  if (deadlineActive(now, nextPetEventEligible)) return;
  if (!mainScreenReadyForPetEvent()) return;
  uint8_t phase = currentDayPhase();
  uint8_t chance = (phase == 0) ? 14 : (phase == 3 ? 8 : 10);
  if ((uint8_t)random(100) >= chance) return;

  petEventType = (uint8_t)random(3);
  petEventUntil = now + PET_EVENT_PROMPT_MS;
  scheduleNextPetEvent(now);
  sfxPlay(SFX_EVENT_SPARKLE);
}

bool inPetEventHit(int16_t x, int16_t y) {
  int16_t ex = 366, ey = 286;
  int dx = x - ex, dy = y - ey;
  return dx * dx + dy * dy <= 44 * 44;
}

void acceptPetEvent() {
  uint8_t type = petEventType;
  petEventUntil = 0;
  scheduleNextPetEvent(millis());
  if (!pet.applyPetEvent(type)) return;
  StrId msg = S_EVENT_FOUND;
  if (type == PET_EVENT_HEART) msg = S_EVENT_PET;
  else if (type == PET_EVENT_SPARKLE) msg = S_EVENT_LUCKY;
  snprintf(petEventMsg, sizeof(petEventMsg), "%s", T(msg));
  petEventFeedbackUntil = millis() + 1800;
  sfxPlay(type == PET_EVENT_BERRY ? SFX_EAT : (type == PET_EVENT_SPARKLE ? SFX_EVENT_SPARKLE : SFX_HEART));
}

bool mainScreenReadyForShake() {
  if (screenOff || pet.awaitingStarter() || pet.isEgg() || pet.sleeping || pet.ceremony) return false;
  if (battleOpen || gameOpen || gameMenuOpen || strengthTrainingScreen.isOpen() || dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return false;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil) return false;
  if (pet.evolving()) return false;
  return true;
}

void maybeOfferMorning(uint32_t now) {
  if (morningUntil) {
    if (deadlineReached(now, morningUntil)) morningUntil = 0;
    return;
  }
  if (screenOff || pet.awaitingStarter() || pet.ceremony) return;
  if (battleOpen || gameOpen || gameMenuOpen || strengthTrainingScreen.isOpen() || cardOpen || dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return;
  if (feedMenuUntil || confirmUntil || choiceKind || bathUntil || wildPromptUntil || petEventUntil) return;
  if (currentDayPhase() != 0) return;
  if (!pet.takeMorningGreeting()) return;
  morningUntil = now + 2500UL;
  if (!pet.sleeping && !pet.isEgg() && audioMode() == SOUND_FULL)
    speciesChirpPlay(pet.speciesId, true);
  else if (!pet.sleeping && !pet.isEgg())
    sfxPlay(SFX_HEART);
}

void closeBattle() {
  battleOpen = false;
  battleResolved = false;
  battleAttackMenuUntil = 0;
  battleCatchOffered = false;
  battleCatchTried = false;
  battleCatchDone = false;
  battleCatchSuccess = false;
  battleRespectCatch = false;
  battleCatchChance = 0;
  wildPmd.unload();
  if (activeScreen == &battleScreen) setScreen(nullptr);
  else { markUiDirty(); lockTouchBrief(); }
}

void startBattleWith(int16_t forcedDex, uint8_t forcedLevel, int8_t forcedShiny) {
  if (!canStartWildBattle(pet.isEgg(), pet.sleeping, pet.ceremony)) return;
  wildPromptUntil = 0;
  scheduleNextWild(millis());
  if (forcedDex >= 1 && forcedDex <= DEX_COUNT) {
    battleDex = forcedDex;
    battleLevel = forcedLevel ? forcedLevel : wildLevelFor(pet.level(), (uint8_t)random(100));
  } else {
    uint8_t speciesRoll = random(100);
    uint8_t levelRoll = random(100);
    battleDex = pickWildSpecies(speciesRoll, currentDayPhase());
    battleLevel = wildLevelFor(pet.level(), levelRoll);
  }
  battleShiny = forcedShiny < 0 ? (random(4096) < pet.stepShinyChancePer4096()) : forcedShiny != 0;
  battlePlayer = petBattleStats();
  battleEnemy = wildBattleStats(battleDex, battleLevel);
  battleEnemy.hp = 0;
  battleResult = {};
  battleRun = beginBattleRuntime(battlePlayer, battleEnemy);
  battleTurn = {};
  battleReward = {};
  battleMsg[0] = 0;
  battleAttackMenuUntil = 0;
  battleLowHpWarned = false;
  battleCatchOffered = false;
  battleCatchTried = false;
  battleCatchDone = false;
  battleCatchSuccess = false;
  battleRespectCatch = false;
  battleCatchChance = 0;
  battleResolved = false;
  battleOpen = true;
  battleDirty = true;
  wildPmd.unload();
  wildPmd.load(battleDex, battleShiny);
  setScreen(&battleScreen);
  sfxPlay(SFX_TAP);
  speciesChirpPlay(battleDex);
}

void startBattle() {
  startBattleWith(0, 0, -1);
}

void finishBattle() {
  if (battleResolved) return;
  battleResolved = true;
  battleDirty = true;
  if (battleTurn.playerWon) {
    bool closeWin = battleRun.playerHp <= battleRun.playerMaxHp / 3;
    battleReward = pet.applyBattleWin(battleDex, closeWin);
    battleCatchOffered = true;
    battleRespectCatch = false;
    battleCatchChance = pet.catchChanceForWild(battleDex, battleLevel, battlePlayer.level, closeWin);
    sfxPlay(SFX_BATTLE_WIN);
  } else {
    battleReward = {};
    pet.applyBattleLoss();
    bool closeLoss = battleRun.enemyHp > 0 && battleRun.enemyHp * 100UL <= battleRun.enemyMaxHp * 30UL;
    battleCatchChance = closeLoss ? pet.respectCatchChanceForWild(battleDex, battleLevel, battlePlayer.level) : 0;
    battleCatchOffered = battleCatchChance > 0;
    battleRespectCatch = battleCatchOffered;
    sfxPlay(SFX_BATTLE_LOSS);
  }
}

void performBattleAction(BattleAction action) {
  if (battleResolved) return;
  battleAttackMenuUntil = 0;
  battleLastAction = action;
  battleTurn = stepBattle(battleRun, action, (uint8_t)random(100));
  if (battleTurn.restFailed) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_NO_REST));
  } else if (battleTurn.counterReady) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_COUNTER_READY));
  } else if (battleTurn.playerRested) {
    char healMsg[18];
    snprintf(healMsg, sizeof(healMsg), T(S_RESTED_FMT), battleTurn.playerHeal);
    snprintf(battleMsg, sizeof(battleMsg), "%s %s", healMsg, T(S_GUARD));
  } else if (battleTurn.playerDamage > 0) {
    if (battleTurn.playerTypePct > 100) snprintf(battleMsg, sizeof(battleMsg), "%s %u", T(S_EFFECTIVE), battleTurn.playerDamage);
    else if (battleTurn.playerTypePct < 100) snprintf(battleMsg, sizeof(battleMsg), "%s %u", T(S_NOT_EFFECTIVE), battleTurn.playerDamage);
    else snprintf(battleMsg, sizeof(battleMsg), T(S_HIT_FMT), battleTurn.playerDamage);
  } else if (battleTurn.enemyDodged) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_ENEMY_DODGED));
  } else if (battleTurn.playerDodged) {
    snprintf(battleMsg, sizeof(battleMsg), T(S_DODGED));
  } else {
    snprintf(battleMsg, sizeof(battleMsg), T(S_MISSED));
  }
  if (battleTurn.battleEnded) {
    finishBattle();
    return;
  }
  if (!battleLowHpWarned && battleRun.playerHp > 0 && battleRun.playerHp <= battleRun.playerMaxHp * 3 / 10) {
    battleLowHpWarned = true;
    sfxPlay(SFX_LOW_HP);
  }
  if (battleTurn.restFailed) sfxPlay(SFX_DENY);
  else if (battleTurn.counterReady) sfxPlay(SFX_COUNTER);
  else if (battleTurn.playerRested) sfxPlay(SFX_REST);
  else if (action == BATTLE_ATTACK_QUICK) sfxPlay(SFX_ATTACK_QUICK);
  else if (action == BATTLE_ATTACK_HEAVY) sfxPlay(SFX_ATTACK_HEAVY);
  else if (battleTurn.playerDamage > 0 && battleTurn.playerTypePct > 100) sfxPlay(SFX_EFFECTIVE);
  else if (battleTurn.playerDamage > 0 && battleTurn.playerTypePct < 100) sfxPlay(SFX_WEAK_HIT);
  else if (battleTurn.enemyDamage > 0) sfxPlay(SFX_ENEMY_HIT);
  else sfxPlay(battleTurn.playerDamage > 0 ? SFX_PLAY : SFX_TAP);
}

const char *battleTypeName(uint8_t type) {
  static const char *const TYPE_NAMES[LANG_COUNT][19] = {
    { "", "NORMAL", "FUEGO", "AGUA", "ELEC", "PLANTA", "HIELO", "LUCHA", "VENENO", "TIERRA", "VUELO", "PSI", "BICHO", "ROCA", "FANT", "DRAGON", "SINIE", "ACERO", "HADA" },
    { "", "NORMAL", "FIRE", "WATER", "ELEC", "GRASS", "ICE", "FIGHT", "POISON", "GROUND", "FLY", "PSY", "BUG", "ROCK", "GHOST", "DRAGON", "DARK", "STEEL", "FAIRY" },
    { "", "NORMAL", "FEU", "EAU", "ELEC", "PLANTE", "GLACE", "COMBAT", "POISON", "SOL", "VOL", "PSY", "INSECT", "ROCHE", "SPECTRE", "DRAGON", "TENEBR", "ACIER", "FEE" },
    { "", "NORMAL", "FEUER", "WASSER", "ELEKTRO", "PFLANZE", "EIS", "KAMPF", "GIFT", "BODEN", "FLUG", "PSYCHO", "KAEFER", "GESTEIN", "GEIST", "DRACHE", "UNLICHT", "STAHL", "FEE" },
    { "", "NORMALE", "FUOCO", "ACQUA", "ELETTRO", "ERBA", "GHIACCIO", "LOTTA", "VELENO", "TERRA", "VOLANTE", "PSICO", "COLEOT", "ROCCIA", "SPETTRO", "DRAGO", "BUIO", "ACCIAIO", "FOLLETTO" },
    { "", "NORMAL", "FOGO", "AGUA", "ELETR", "PLANTA", "GELO", "LUTA", "VENENO", "TERRA", "VOO", "PSI", "INSETO", "PEDRA", "FANT", "DRAGAO", "SOMBRIO", "ACO", "FADA" },
  };
  if (type < 19) return TYPE_NAMES[gLang][type];
  switch (type) {
    case TYPE_NORMAL: return "NORMAL";
    case TYPE_FIRE: return "FIRE";
    case TYPE_WATER: return "WATER";
    case TYPE_ELECTRIC: return "ELEC";
    case TYPE_GRASS: return "GRASS";
    case TYPE_ICE: return "ICE";
    case TYPE_FIGHTING: return "FIGHT";
    case TYPE_POISON: return "POISON";
    case TYPE_GROUND: return "GROUND";
    case TYPE_FLYING: return "FLY";
    case TYPE_PSYCHIC: return "PSY";
    case TYPE_BUG: return "BUG";
    case TYPE_ROCK: return "ROCK";
    case TYPE_GHOST: return "GHOST";
    case TYPE_DRAGON: return "DRAGON";
    case TYPE_DARK: return "DARK";
    case TYPE_STEEL: return "STEEL";
    case TYPE_FAIRY: return "FAIRY";
  }
  return "";
}

uint16_t battleTypeColor(uint8_t type) {
  switch (type) {
    case TYPE_FIRE: return 0xEA87;
    case TYPE_WATER: return 0x4C98;
    case TYPE_ELECTRIC: return 0xBCA1;
    case TYPE_GRASS: return 0x3C49;
    case TYPE_ICE: return 0x5D99;
    case TYPE_FIGHTING: return 0xA2A5;
    case TYPE_POISON: return 0x8A73;
    case TYPE_GROUND: return 0xB447;
    case TYPE_FLYING: return 0x8D7F;
    case TYPE_PSYCHIC: return 0xD28F;
    case TYPE_BUG: return 0x7CC4;
    case TYPE_ROCK: return 0x9407;
    case TYPE_GHOST: return 0x6B33;
    case TYPE_DRAGON: return 0x5A5F;
    case TYPE_DARK: return 0x5ACB;
    case TYPE_STEEL: return 0xA534;
    case TYPE_FAIRY: return 0xF3B7;
    default: return 0x8C4D;
  }
}

void typeText(char *buf, size_t len, const DexEntry &d) {
  if (d.type2 == TYPE_NONE) snprintf(buf, len, "%s", battleTypeName(d.type1));
  else snprintf(buf, len, "%s %s", battleTypeName(d.type1), battleTypeName(d.type2));
}

int typeChipWidth(uint8_t type) {
  return (int)strlen(battleTypeName(type)) * 6 + 14;
}

void drawTypeChip(int x, int y, uint8_t type) {
  if (type == TYPE_NONE) return;
  const char *label = battleTypeName(type);
  int w = typeChipWidth(type);
  gfx->fillRoundRect(x, y, w, 16, 5, lerp565(battleTypeColor(type), UI_WHITE, 5, 8));
  gfx->drawRoundRect(x, y, w, 16, 5, UI_INK);
  gfx->setTextSize(1);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(x + 7, y + 5);
  gfx->print(label);
}

int drawTypeChips(int x, int y, const DexEntry &d, bool alignRight) {
  int w1 = typeChipWidth(d.type1);
  int w2 = d.type2 == TYPE_NONE ? 0 : typeChipWidth(d.type2);
  int total = w1 + (w2 ? 4 + w2 : 0);
  int sx = alignRight ? x - total : x;
  drawTypeChip(sx, y, d.type1);
  if (d.type2 != TYPE_NONE) drawTypeChip(sx + w1 + 4, y, d.type2);
  gfx->setTextSize(2);
  return sx;
}

void drawWildPrompt() {
  gfx->fillRoundRect(82, 156, 302, 178, 18, UI_WHITE);
  gfx->drawRoundRect(82, 156, 302, 178, 18, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(T(S_WILD_Q)) * 9, 176);
  gfx->print(T(S_WILD_Q));
  char name[28];
  snprintf(name, sizeof(name), "%s%s Lv.%u", wildPromptShiny ? "*" : "",
           dexName(wildPromptDex), wildPromptLevel);
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(name) * 6, 206);
  gfx->print(name);

  gfx->fillRoundRect(93, 226, 280, 44, 12, UI_BAR_BAD);
  gfx->fillRoundRect(93, 278, 280, 44, 12, UI_TRACK);
  gfx->setTextColor(UI_WHITE);
  gfx->setCursor(CX - strlen(T(S_FIGHT)) * 6, 240);
  gfx->print(T(S_FIGHT));
  gfx->setTextColor(UI_BG_DAY);
  gfx->setCursor(CX - strlen(T(S_LATER)) * 6, 292);
  gfx->print(T(S_LATER));
}

void drawPetEvent() {
  uint32_t now = millis();
  if (petEventUntil && deadlineReached(now, petEventUntil)) petEventUntil = 0;
  if (!petEventUntil && (!petEventFeedbackUntil || deadlineReached(now, petEventFeedbackUntil))) return;

  int16_t ex = 366, ey = 286;
  if (petEventUntil) {
    gfx->fillCircle(ex, ey, 32, UI_WHITE);
    gfx->drawCircle(ex, ey, 34, UI_BAR_WARN);
    if (petEventType == PET_EVENT_BERRY) {
      drawMap(SPR_ICON_BERRY_G, 16, ex - 24, ey - 24, 3, false);
    } else if (petEventType == PET_EVENT_HEART) {
      drawMap(SPR_HEART, 32, ex - 32, ey - 32, 2, false);
    } else {
      gfx->setTextColor(UI_BAR_WARN);
      gfx->setTextSize(4);
      gfx->setCursor(ex - 12, ey - 18);
      gfx->print("*");
      gfx->fillCircle(ex - 14, ey + 12, 4, UI_WHITE);
      gfx->fillCircle(ex + 16, ey + 10, 4, UI_WHITE);
    }
  }
  if (petEventFeedbackUntil && deadlineActive(now, petEventFeedbackUntil)) {
    gfx->setTextColor(UI_BAR_WARN);
    gfx->setTextSize(2);
    gfx->setCursor(CX - strlen(petEventMsg) * 6, 292);
    gfx->print(petEventMsg);
  }
}

// ---------- ficha del bicho (deslizar vertical) ----------

// ---------- ajuste de hora en pantalla (deslizar abajo) ----------
// El usuario pone su hora LOCAL a ojo; el firmware la usa tal cual, asi que
// no hay que gestionar zona horaria. Preserva el dia (no rompe racha/edad).

void openClock() {
  uint32_t e = pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch();
  clockH = (e / 3600) % 24;
  clockM = (e / 60) % 60;
  clockOpen = true;
  clockDirty = true;
  setScreen(&clockSettingsScreen);
  sfxPlay(SFX_MENU);
}

void openHelp() {
  helpPage = 0;
  helpOpen = true;
  clockOpen = false;
  helpDirty = true;
  setScreen(&helpScreen);
  sfxPlay(SFX_MENU);
}

// llama + numero de racha arriba a la izquierda
void drawStreakBadge() {
  if (pet.streak < 1) return;
  int x = 26, y = 16;
  gfx->fillTriangle(x + 8, y, x + 1, y + 17, x + 15, y + 17, UI_BAR_BAD);
  gfx->fillTriangle(x + 8, y + 7, x + 4, y + 17, x + 12, y + 17, UI_BAR_WARN);
  char s[6];
  snprintf(s, sizeof(s), "%u", pet.streak);
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(x + 22, y + 2);
  gfx->print(s);
}

// banner temporal: medalla nueva o hito de racha
void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  char buf[20];
  if (pet.showMedal()) {
    for (int i = 0; i < MED_COUNT; i++)
      if (pet.newMedal & (1 << i)) { l2 = medalName(i); break; }
    l1 = T(S_MEDAL_BANNER);
  } else if (pet.showDexReward()) {
    l1 = T(S_NEW_FRAME);
    l2 = T((StrId)(S_RANK_TRAINER + pet.collectionRank()));
  } else if (pet.showMilestone()) {
    snprintf(buf, sizeof(buf), T(S_STREAK_DAYS_FMT), pet.streak);
    l1 = T(S_GREAT);
    l2 = buf;
  } else if (pet.showStepReward()) {
    l1 = T(S_STEP_REWARD);
    if (pet.lastStepReward() == STEP_REWARD_TRAIL_RANK) {
      snprintf(buf, sizeof(buf), T(S_TRAIL_RANK_FMT), pet.stepTrailRank());
      l2 = buf;
    } else if (pet.lastStepReward() == STEP_REWARD_ENERGY) {
      l2 = T(S_ITEM_ENERGY);
    } else if (pet.lastStepReward() == STEP_REWARD_TRAIN) {
      l2 = T(S_ITEM_TRAIN);
    } else {
      l2 = T(S_ITEM_SNACK);
    }
  }
  if (!l1) return;
  gfx->fillRoundRect(73, 150, 320, 96, 16, UI_BAR_WARN);
  gfx->drawRoundRect(73, 150, 320, 96, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(l1) * 9, 176);
  gfx->print(l1);
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(l2) * 6, 212);
  gfx->print(l2);
}

StrId expeditionItemText(ExpeditionItem item) {
  switch (item) {
    case EXP_ITEM_SNACK: return S_ITEM_SNACK;
    case EXP_ITEM_ENERGY: return S_ITEM_ENERGY;
    case EXP_ITEM_CARE: return S_ITEM_CARE;
    case EXP_ITEM_TRAIN: return S_ITEM_TRAIN;
    default: return S_WAIT;
  }
}

StrId expeditionSiteText(ExpeditionSite site) {
  if (site == EXP_SITE_VOLCANO) return S_EXP_VOLCANO;
  if (site == EXP_SITE_FOREST) return S_EXP_FOREST;
  return S_EXP_COAST;
}

uint16_t expeditionSiteColor(ExpeditionSite site) {
  return site == EXP_SITE_VOLCANO ? UI_BAR_BAD : site == EXP_SITE_FOREST ? UI_BAR_OK : 0x4C98;
}

uint16_t expeditionItemColor(ExpeditionItem item) {
  switch (item) {
    case EXP_ITEM_SNACK: return UI_BAR_WARN;
    case EXP_ITEM_ENERGY: return 0x4C98;
    case EXP_ITEM_CARE: return UI_BAR_OK;
    case EXP_ITEM_TRAIN: return UI_BAR_BAD;
    default: return UI_TRACK;
  }
}

uint32_t expeditionNowEpoch() {
  uint32_t nowEpoch = rtcEpoch();
  return nowEpoch ? nowEpoch : pet.lastSeenEpoch;
}

static const int EXP_HUD_X = 310;
static const int EXP_HUD_Y = 106;
static const int EXP_HUD_W = 112;
static const int EXP_HUD_H = 34;

// Der obere Rand des runden Displays ist an den Ecken nicht sichtbar. Der
// Schritt-Chip sitzt deshalb links ueber dem Namen, innerhalb der Kreisflaeche;
// rechts bleibt der Expeditions-Hinweis frei.
static const int STEP_HUD_X = 130;
static const int STEP_HUD_Y = 24;
static const int STEP_HUD_W = 126;
static const int STEP_HUD_H = 22;

bool inStepsHudHit(int16_t x, int16_t y) {
  return x >= STEP_HUD_X && x <= STEP_HUD_X + STEP_HUD_W &&
         y >= STEP_HUD_Y && y <= STEP_HUD_Y + STEP_HUD_H;
}

void openStepsCard() {
  cardOpen = true;
  cardPage = 8;
  expeditionTrainChoiceOpen = false;
  cardDirty = true;
  setScreen(petCardScreenForPage(cardPage));
  sfxPlay(SFX_MENU);
}

void drawStepsHud() {
  uint16_t color = pet.stepsToday >= 5000UL ? UI_BAR_WARN :
                   pet.stepsToday >= 500UL ? UI_BAR_OK : UI_TRACK;
  char label[24];
  snprintf(label, sizeof(label), "%s %lu", T(S_STEPS), (unsigned long)pet.stepsToday);
  gfx->fillRoundRect(STEP_HUD_X, STEP_HUD_Y, STEP_HUD_W, STEP_HUD_H, 8, UI_WHITE);
  gfx->drawRoundRect(STEP_HUD_X, STEP_HUD_Y, STEP_HUD_W, STEP_HUD_H, 8, color);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(STEP_HUD_X + (STEP_HUD_W - (int)strlen(label) * 6) / 2, STEP_HUD_Y + 10);
  gfx->print(label);
}

bool expeditionHudVisible() {
  if (pet.isEgg() || pet.ceremony || cardOpen || gameOpen || strengthTrainingScreen.isOpen() || battleOpen ||
      dexScreen.isOpen() || kbOpen || clockOpen || helpOpen) return false;
  if (gameMenuOpen || feedMenuUntil || confirmUntil || choiceKind || bathUntil ||
      petEventUntil || wildPromptUntil) return false;
  return pet.expeditionHudState(expeditionNowEpoch()) != EXP_HUD_HIDDEN;
}

bool inExpeditionHudHit(int16_t x, int16_t y) {
  return expeditionHudVisible() && x >= EXP_HUD_X && x <= EXP_HUD_X + EXP_HUD_W &&
         y >= EXP_HUD_Y && y <= EXP_HUD_Y + EXP_HUD_H;
}

void openExpeditionCard() {
  cardOpen = true;
  cardPage = 7;
  expeditionTrainChoiceOpen = false;
  cardDirty = true;
  setScreen(petCardScreenForPage(cardPage));
  sfxPlay(SFX_MENU);
}

void drawExpeditionHud() {
  if (!expeditionHudVisible()) return;

  uint32_t nowEpoch = expeditionNowEpoch();
  ExpeditionHudState state = pet.expeditionHudState(nowEpoch);
  uint16_t color = state == EXP_HUD_READY ? UI_BAR_OK :
                   state == EXP_HUD_BAG ? UI_BAR_WARN : 0x4C98;
  char label[18];
  if (state == EXP_HUD_ACTIVE) {
    uint32_t left = (pet.expeditionEndEpoch - nowEpoch + 59UL) / 60UL;
    snprintf(label, sizeof(label), "%s %lum", T(expeditionSiteText((ExpeditionSite)pet.expeditionSite)), (unsigned long)left);
  } else if (state == EXP_HUD_READY) {
    snprintf(label, sizeof(label), "%s", T(S_EXP_READY));
  } else {
    snprintf(label, sizeof(label), "%s x%u", T(S_EXP_HUD_BAG), pet.expeditionItemCount());
  }

  gfx->fillRoundRect(EXP_HUD_X, EXP_HUD_Y, EXP_HUD_W, EXP_HUD_H, 8, UI_WHITE);
  gfx->drawRoundRect(EXP_HUD_X, EXP_HUD_Y, EXP_HUD_W, EXP_HUD_H, 8, color);
  gfx->fillCircle(EXP_HUD_X + 17, EXP_HUD_Y + 17, 9, color);
  gfx->setTextColor(UI_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(EXP_HUD_X + 14, EXP_HUD_Y + 12);
  if (state == EXP_HUD_READY) gfx->print("!");
  else if (state == EXP_HUD_BAG) gfx->print("+");
  else gfx->print(">");
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(1);
  gfx->setCursor(EXP_HUD_X + 31, EXP_HUD_Y + 13);
  gfx->print(label);
}

// ---------- teclado para renombrar ----------

void openKeyboard() {
  kbOpen = true;
  keyboardDirty = true;
  setScreen(&keyboardScreen);
  strncpy(nameBuf, pet.nick, sizeof(nameBuf) - 1);
  nameBuf[sizeof(nameBuf) - 1] = 0;
  nameLen = strlen(nameBuf);
  sfxPlay(SFX_MENU);
}

// ---------- galeria pokedex ----------

#define GAL_X 73
#define GAL_Y 116
#define GAL_CELL 74

// dibuja una miniatura centrada en su celda; sil=true la pinta en tinta
void drawThumb(const uint8_t *b, int x, int y, int s, bool sil) {
  uint8_t w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3;
  const uint8_t *d = pal + n * 2;
  int ox = x + (GAL_CELL - w * s) / 2;
  int oy = y + (GAL_CELL - h * s) / 2;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      uint8_t idx = d[r * w + c];
      if (idx == 0xFF) continue;
      uint16_t col = sil ? INK_K : (uint16_t)(pal[idx * 2] | (pal[idx * 2 + 1] << 8));
      gfx->fillRect(ox + c * s, oy + r * s, s, s, col);
    }
  }
}

void drawBattery() {
  int pc = batPercent();
  if (pc < 0) return;  // sin bateria conectada
  int x = CX - 14, y = 12, w = 24, h = 11;
  bool charging = batCharging();
  uint16_t col = charging ? UI_BAR_OK
                 : (pc >= 40) ? inkColor()
                 : (pc >= 15) ? UI_BAR_WARN
                              : UI_BAR_BAD;
  gfx->drawRoundRect(x, y, w, h, 2, col);
  gfx->fillRect(x + w, y + 3, 3, 5, col);  // borne
  if (charging) {
    // rayo de carga (zigzag) en vez de la barra de nivel
    uint16_t bolt = C565(0xff, 0xd9, 0x4a);
    int bx = x + w / 2;
    gfx->fillTriangle(bx + 3, y + 1, bx - 4, y + 6, bx + 1, y + 6, bolt);
    gfx->fillTriangle(bx - 1, y + 5, bx + 4, y + 5, bx - 3, y + 10, bolt);
  } else {
    int fw = (w - 4) * pc / 100;
    if (fw > 0) gfx->fillRect(x + 2, y + 2, fw, h - 4, col);
  }
}

void drawHeader(const char *name, uint16_t nameColor, const char *msg) {
  drawBattery();
  gfx->setTextColor(nameColor);
  gfx->setTextSize(3);
  gfx->setCursor(CX - strlen(name) * 9, 52);
  gfx->print(name);
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(CX - strlen(msg) * 6, 90);
  gfx->print(msg);
}

// animacion de la ceremonia (10s): despedida = reverencia con corazones y se
// aleja caminando; escapada = se asusta y sale corriendo. Sustituye al idle.
void drawCeremony() {
  if (!pmd.loaded) { drawPet(); return; }  // respaldo si no hay sprite PMD
  uint32_t now = millis();
  float t = pet.ceremonyT();               // 0..1 a lo largo de los 10s
  bool panic = (pet.ceremony == CER_RUNAWAY);
  int x = CX, y = PET_GROUND;
  uint8_t act = PMD_IDLE;

  if (panic) {
    // final triste: penumbra azulada + lluvia
    for (int i = 0; i < 46; i++) {
      int rx = (i * 47 + now / 3) % 466;
      int ry = (i * 91 + now / 2) % 470;
      gfx->drawLine(rx, ry, rx - 3, ry + 12, C565(0x6a, 0x84, 0xb0));
    }
    bool fade = false;
    if (t < 0.30f) {                       // cabizbajo, temblando
      act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
      x = CX + (int)(4 * sinf(now * 0.04f));
    } else {                               // se aleja despacio y se desvanece
      act = pmd.has(PMD_WALKL) ? PMD_WALKL : PMD_IDLE;
      x = CX - (int)(((t - 0.30f) / 0.70f) * (CX + 120));
      fade = (t > 0.6f) && ((now / 160) % 2 == 0);  // parpadea hacia la silueta
    }
    drawPmdAct(act, x, y, now, true, fade, 5);  // fade=silueta: se difumina al irse
    // lagrima cayendo del bicho
    if (t < 0.55f) {
      int ty = y - 150 + (int)((now / 6) % 40);
      gfx->fillRect(x + 6, ty, 3, 6, C565(0x9a, 0xc4, 0xe8));
    }
    return;
  }

  // despedida epica: halo dorado pulsante + chispas y corazones que ascienden
  int gcy = PET_GROUND - 96;
  for (int k = 0; k < 4; k++) {
    int r = 60 + k * 34 + (int)(10 * sinf(now * 0.02f));
    gfx->drawCircle(CX, gcy, r, C565(0xff, 0xdf, 0x8a));
  }
  for (int i = 0; i < 16; i++) {
    int px = (i * 71 + 28) % 466;
    int py = 410 - (int)((now / 8 + i * 70) % 360);   // suben y reaparecen abajo
    if (py < 30) continue;
    if (i % 4 == 0) drawMap(SPR_HEART, 32, px - 8, py - 8, 1, false);  // corazoncito
    else gfx->fillRect(px, py, 4, 4, (i % 2) ? C565(0xff, 0xe7, 0x9f) : C565(0xff, 0x9a, 0xc0));
  }

  if (t < 0.45f) {                         // reverencia / pose de despedida
    act = pmd.has(PMD_POSE) ? PMD_POSE : (pmd.has(PMD_NOD) ? PMD_NOD : PMD_IDLE);
  } else {                                 // se aleja por la derecha
    act = pmd.has(PMD_WALKR) ? PMD_WALKR : PMD_IDLE;
    x = CX + (int)(((t - 0.45f) / 0.55f) * (CX + 140));
  }
  drawPmdAct(act, x, y, now, true, false, 5);
  if (pet.showHeart())                     // corazon grande siguiendo al bicho
    drawMap(SPR_HEART, 32, x + 50, y - 190, 2, false);
}

// dialogo de decision (2 botones apilados): evolucionar/mantener o despedirse/quedaros
void drawChoiceDialog() {
  if (choiceKind == 3) {
    gfx->fillRoundRect(52, 96, 356, 350, 18, UI_WHITE);
    gfx->drawRoundRect(52, 96, 356, 350, 18, UI_INK);
    gfx->setTextColor(UI_INK);
    gfx->setTextSize(2);
    const char *title = T(S_EVO_Q);
    gfx->setCursor(CX - (int)strlen(title) * 6, 116);
    gfx->print(title);
    uint8_t options = pet.evolutionOptionCount();
    for (uint8_t i = 0; i < options; i++) {
      int16_t target = pet.evolutionOption(i);
      int y = 158 + i * 45;
      gfx->fillRoundRect(82, y, 312, 36, 10, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      const char *name = dexName(target);
      gfx->setCursor(CX - (int)strlen(name) * 6, y + 11);
      gfx->print(name);
    }
    gfx->fillRoundRect(82, 392, 312, 40, 10, UI_TRACK);
    gfx->setTextColor(UI_INK);
    const char *cancel = T(S_EVO_KEEP);
    gfx->setCursor(CX - (int)strlen(cancel) * 6, 404);
    gfx->print(cancel);
    return;
  }
  const char *q, *o1, *o2;
  uint16_t c1, c2, t1, t2;
  if (choiceKind == 1) {  // evolucion
    q = T(S_EVO_Q); o1 = T(S_EVO_TAP); o2 = T(S_EVO_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else {                // despedida
    q = T(S_FAR_Q); o1 = T(S_FAR_GO); o2 = T(S_FAR_STAY);
    c1 = UI_BAR_WARN; t1 = UI_INK; c2 = UI_BAR_OK; t2 = UI_WHITE;
  }
  gfx->fillRoundRect(73, 156, 320, 188, 16, UI_WHITE);
  gfx->drawRoundRect(73, 156, 320, 188, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(q) * 6, 176);
  gfx->print(q);
  gfx->fillRoundRect(93, 206, 280, 52, 12, c1);     // boton accion
  gfx->setTextColor(t1);
  gfx->setCursor(CX - (int)strlen(o1) * 6, 224);
  gfx->print(o1);
  gfx->fillRoundRect(93, 268, 280, 52, 12, c2);     // boton mantener/quedaros
  gfx->setTextColor(t2);
  gfx->setCursor(CX - (int)strlen(o2) * 6, 286);
  gfx->print(o2);
}

// boton-CTA rojo y grande para evolucionar (pulsa para llamar la atencion)
void drawEvolveButton() {
  uint32_t now = millis();
  int p = (int)(5 * sinf(now * 0.006f));  // late: -5..5
  int x = EVO_BTN_X - p, y = EVO_BTN_Y - p, w = EVO_BTN_W + 2 * p, h = EVO_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 18, UI_BAR_BAD);
  gfx->drawRoundRect(x, y, w, h, 18, UI_WHITE);
  gfx->drawRoundRect(x + 2, y + 2, w - 4, h - 4, 16, UI_WHITE);
  gfx->setTextColor(UI_WHITE);
  gfx->setTextSize(3);
  const char *t = T(S_EVO_TAP);
  gfx->setCursor(CX - (int)strlen(t) * 9, y + h / 2 - 11);
  gfx->print(t);
}

// boton-CTA dorado de despedida: "<nombre> quiere decirte algo..."
void drawFarewellButton() {
  uint32_t now = millis();
  int p = (int)(4 * sinf(now * 0.005f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, UI_BAR_WARN);
  gfx->drawRoundRect(x, y, w, h, 16, UI_INK);
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_FAREWELL_BTN), nm);
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(buf) * 6, y + h / 2 - 8);
  gfx->print(buf);
}

// boton-CTA sombrio de escapada por abandono: "<nombre> se siente abandonado..."
// (final triste: azul-gris oscuro, latido lento y apagado)
void drawRunawayButton() {
  uint32_t now = millis();
  int p = (int)(3 * sinf(now * 0.003f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, C565(0x3a, 0x44, 0x5a));
  gfx->drawRoundRect(x, y, w, h, 16, C565(0x70, 0x80, 0x98));
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_RUNAWAY_BTN), nm);
  gfx->setTextColor(C565(0xc8, 0xd2, 0xe0));
  gfx->setTextSize(2);
  gfx->setCursor(CX - (int)strlen(buf) * 6, y + h / 2 - 8);
  gfx->print(buf);
}

// animacion epica de evolucion: halo radial + rayos giratorios + parpadeo del
// sprite acelerando + chispas que salen disparadas + fogonazo final
void drawEvolveFX(uint32_t now) {
  float t = pet.evolveT();          // 0..1
  int cx = CX, cy = PET_GROUND - 96;

  // halo radial que crece y pulsa
  int halo = 36 + (int)(t * 150) + (int)(8 * sinf(now * 0.02f));
  for (int k = 0; k < 4; k++) {
    int r = halo - k * 7;
    if (r > 0) gfx->drawCircle(cx, cy, r, UI_WHITE);
  }
  // rayos giratorios desde el centro del bicho
  float base = now * 0.004f;
  for (int i = 0; i < 12; i++) {
    float a = base + i * (float)(PI / 6);
    int len = 90 + (int)(70 * (0.5f + 0.5f * sinf(now * 0.012f + i)));
    gfx->drawLine(cx, cy, cx + (int)(cosf(a) * len), cy + (int)(sinf(a) * len), UI_WHITE);
  }
  // parpadeo entre la forma ANTERIOR y la NUEVA (siluetas), acelerando; al
  // final (t>0.9) se queda fija en la nueva para el fogonazo de revelado
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && evoPmd.loaded && ((now / period) % 2) == 0;
  if (showOld) drawPmdActM(evoPmd, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  // chispas que salen disparadas
  for (int i = 0; i < 10; i++) {
    float a = i * (float)(PI / 5) + t * 4.0f;
    int d = (int)((now / 14 + i * 33) % 200);
    int sx = cx + (int)(cosf(a) * d), sy = cy + (int)(sinf(a) * d);
    gfx->fillRect(sx - 2, sy - 2, 5, 5, (i & 1) ? C565(0xff, 0xe0, 0x70) : UI_WHITE);
  }
  // fogonazo final antes de revelar la forma nueva
  if (t > 0.9f) gfx->fillCircle(cx, cy, (int)(300 * (t - 0.9f) / 0.1f), UI_WHITE);
}

void drawPet() {
  if (pmd.loaded) {
    drawPetPMD();
    return;
  }
  if (mon.loaded) {
    drawPetSD();
    return;
  }
  int fi = flashIdxForDex(pet.speciesId);
  if (fi < 0) {
    // sin SD y sin sprite de flash: aviso claro de que faltan sprites
    gfx->setTextColor(inkColor());
    gfx->setTextSize(6);
    gfx->setCursor(CX - 18, PET_CY - 80);
    gfx->print("?");
    gfx->setTextSize(2);
    const char *l1 = T(S_NO_SPRITES);
    gfx->setCursor(CX - (int)strlen(l1) * 6, PET_CY - 4);
    gfx->print(l1);
    const char *l2 = T(S_LOAD_SPRITES);
    gfx->setCursor(CX - (int)strlen(l2) * 6, PET_CY + 20);
    gfx->print(l2);
    return;
  }
  const Species &sp = SPECIES[fi];
  int s = sp.scale;
  int x = CX - 16 * s;
  int y = PET_CY - 16 * s;

  // animacion de evolucion: alterna la silueta de la forma anterior y la nueva
  if (pet.evolving()) {
    bool flash = (millis() / 300) % 2;
    int16_t showDex = (flash && pet.prevSpeciesId >= 0) ? pet.prevSpeciesId : pet.speciesId;
    int sfi = flashIdxForDex(showDex);
    if (sfi >= 0) {
      const Species &show = SPECIES[sfi];
      drawMap(show.sprite, SPRITE_H, CX - 16 * show.scale, PET_CY - 16 * show.scale, show.scale, flash);
    }
    return;
  }

  PetMood m = pet.mood();
  if (m == MOOD_HAPPY && (millis() / 500) % 2) y -= 6;  // saltito

  drawMap(sp.sprite, SPRITE_H, x, y, s, false);

  // expresiones superpuestas usando las anclas de la especie
  bool blink = (millis() % 3500 < 300);
  if (m == MOOD_SLEEPING || blink) {
    overlayEye(sp, x, y, s, sp.eyeColL);
    overlayEye(sp, x, y, s, sp.eyeColR);
  }
  if (m == MOOD_EATING) overlayMouth(sp, x, y, s, true);
  else if (m == MOOD_SAD) overlayMouth(sp, x, y, s, false);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + 20 * s, y - 2 * s, 2, false);
}

// ---------- escena de bano ----------

void startBath() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || bathUntil) return;
  bathUntil = millis() + 3000;
  bathPending = true;
  sfxPlay(SFX_EVENT_SPARKLE);
  int cx = (int)beh.x;
  for (auto &b : bubbles) {
    b.x = cx - 70 + random(140);
    b.y = PET_GROUND - random(150);
    b.r = 8 + random(16);
    b.ph = random(64);
  }
}

void drawBath() {
  if (!bathUntil) return;
  uint32_t now = millis();
  if (deadlineReached(now, bathUntil)) {
    bathUntil = 0;
    if (bathPending) {
      bathPending = false;
      pet.clean();
      // pose de alegria al quedar limpio
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]) * 2;
      }
    }
    return;
  }
  uint32_t left = bathUntil - now;
  if (left > 800) {
    // espuma: pompas meciendose y subiendo poco a poco
    float t = now / 220.0f;
    for (auto &b : bubbles) {
      int bx = b.x + (int)(sinf(t + b.ph) * 6);
      int by = b.y - (int)((3000 - left) / 90);
      gfx->fillCircle(bx, by, b.r, UI_WHITE);
      gfx->drawCircle(bx, by, b.r, 0x7E3D);
      gfx->fillCircle(bx - b.r / 3, by - b.r / 3, b.r / 4, UI_BG_DAY);
    }
  } else {
    // las pompas revientan: destellos
    for (int i = 0; i < 8; i++) {
      auto &b = bubbles[i];
      int sx = b.x + (i % 3) * 6 - 6, sy = b.y - 18;
      uint16_t col = (i % 2) ? UI_BAR_WARN : UI_WHITE;
      gfx->fillRect(sx - 6, sy - 1, 13, 3, col);
      gfx->fillRect(sx - 1, sy - 6, 3, 13, col);
    }
  }
}

// ---------- mascota PMD: comportamiento ----------

uint32_t pmdActTotalMs(const PmdAct &a) {
  uint32_t t = 0;
  for (uint8_t i = 0; i < a.frames; i++) t += a.ms[i];
  return t ? t : 100;
}

uint8_t pmdFrameAt(const PmdAct &a, uint32_t t, bool loop) {
  uint32_t total = pmdActTotalMs(a);
  if (!loop && t >= total) return a.frames - 1;
  t %= total;
  uint8_t i = 0;
  while (t >= a.ms[i]) {
    t -= a.ms[i];
    i = (i + 1) % a.frames;
  }
  return i;
}

// dibuja una accion anclada por la base (centro-x, suelo) y devuelve su escala
// dibuja una accion de un PmdMon concreto (m); drawPmdAct usa el global pmd
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  const PmdAct &a = m.acts[actId];
  if (!a.frames) return;
  uint8_t sBase = m.acts[PMD_IDLE].h ? 170 / m.acts[PMD_IDLE].h : 5;
  if (sBase < 2) sBase = 2;
  if (sBase > maxS) sBase = maxS;
  uint8_t s = sBase;
  while (s > 2 && a.h * s > 250) s--;  // acciones con frame grande (ataque)
  uint8_t fi = pmdFrameAt(a, t, loop);
  const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
  // anclar por los pies (a.base), no por el alto del lienzo: asi las acciones
  // con padding distinto (Hurt, Eat...) quedan todas a la misma altura de suelo
  int x0 = cx - a.w * s / 2, y0 = groundY - (a.base ? a.base : a.h) * s;
  for (int r = 0; r < a.h; r++) {
    const uint8_t *row = fr + r * a.w;
    for (int c = 0; c < a.w; c++) {
      uint8_t idx = row[c];
      if (idx == 0xFF) continue;
      gfx->fillRect(x0 + c * s, y0 + r * s, s, s, sil ? INK_K : m.pal[idx]);
    }
  }
}
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  drawPmdActM(pmd, actId, cx, groundY, t, loop, sil, maxS);
}

// elige el siguiente capricho del bicho cuando esta contento
void behNext() {
  uint32_t now = millis();
  beh.t0 = now;
  int r = random(100);
  uint8_t walkChance = (currentDayPhase() == 3) ? 10 : 35;
  if (r < walkChance && (pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR))) {
    beh.mode = 1;  // paseo
    beh.targetX = 150 + random(176);
    beh.until = now + 15000;
  } else if (r < 60) {
    // gesto aleatorio entre los disponibles
    // (Hop fuera: salta demasiado alto; Sit fuera: mira hacia atras)
    static const uint8_t flair[] = { PMD_POSE, PMD_NOD, PMD_BREATH };
    uint8_t pick[3], n = 0;
    for (uint8_t f : flair)
      if (pmd.has(f)) pick[n++] = f;
    if (n) {
      beh.mode = 2;
      beh.act = pick[random(n)];
      beh.until = now + pmdActTotalMs(pmd.acts[beh.act]);
      return;
    }
    beh.mode = 0;
    beh.until = now + 2000 + random(3000);
  } else {
    beh.mode = 0;  // mirar al frente
    beh.until = now + 2000 + random(3000);
  }
}

void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // termino la evolucion: libera la forma anterior

  PetMood m = pet.mood();
  uint8_t act;
  bool loop = true;
  if (m == MOOD_SLEEPING && pmd.has(PMD_SLEEP)) {
    act = PMD_SLEEP;
    beh.mode = 0;
  } else if (m == MOOD_EATING && pmd.has(PMD_EAT)) {
    act = PMD_EAT;
    beh.t0 = 0;
  } else if (m == MOOD_SAD && pmd.has(PMD_HURT)) {
    act = PMD_HURT;
  } else {
    // contento: el planificador decide (idle / paseo / gesto)
    if (deadlineReached(now, beh.until)) behNext();
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        behNext();
        act = PMD_IDLE;
      } else {
        beh.x += (d > 0 ? 3.0f : -3.0f);
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
      }
    } else {
      act = (beh.mode == 2) ? beh.act : PMD_IDLE;
      loop = false;
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  drawPmdAct(act, (int)beh.x, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, 5);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, (int)beh.x + 50, PET_GROUND - 190, 2, false);
}

// sprite animado desde la SD: zoom entero por pixel, frames a su ritmo
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // saltito
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    for (int c = 0; c < mon.w; c++) {
      uint8_t idx = row[c];
      if (idx == 0xFF) continue;
      gfx->fillRect(x + c * s, y + r * s, s, s, sil ? INK_K : mon.pal[idx]);
    }
  }

  // emotes en vez de expresiones (los sprites importados no tienen anclas)
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
}

// ojo cerrado: borra el ojo 3x4 y dibuja el parpado
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

// borra la sonrisa base y pinta boca abierta (comer) o ceno (triste)
void overlayMouth(const Species &sp, int x, int y, int s, bool open) {
  int mc = sp.mouthCol, mr = sp.mouthRow;
  gfx->fillRect(x + (mc - 3) * s, y + mr * s, 7 * s, 2 * s, sp.bodyColor);
  if (open) {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, 2 * s, INK_K);
  } else {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, s, INK_K);
    gfx->fillRect(x + (mc - 3) * s, y + (mr + 1) * s, s, s, INK_K);
    gfx->fillRect(x + (mc + 3) * s, y + (mr + 1) * s, s, s, INK_K);
  }
}

void drawPoops() {
  for (int i = 0; i < pet.poops; i++) {
    drawMap(SPR_POOP, 32, 36 + i * 46, 244, 2, false);
  }
}

void drawBars() {
  drawBar(78, 318, T(S_BAR_FOOD), pet.fullness);
  drawBar(244, 318, T(S_BAR_JOY), pet.joy);
  drawBar(78, 346, T(S_BAR_ENE), pet.energy);
  drawBar(244, 346, T(S_BAR_HYG), pet.hygiene);
}

void drawBar(int x, int y, const char *label, uint8_t val) {
  gfx->setTextColor(inkColor());
  gfx->setTextSize(2);
  gfx->setCursor(x, y);
  gfx->print(label);
  int bx = x + 48, bw = 100, bh = 15;  // +48: deja sitio a etiquetas de 4 letras (EN)
  uint16_t fill = (val >= 50) ? UI_BAR_OK : (val >= 25) ? UI_BAR_WARN : UI_BAR_BAD;
  gfx->fillRoundRect(bx, y, bw, bh, 4, UI_TRACK);
  int fw = (bw - 4) * val / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, bh - 4, 3, fill);
}

void drawButtons() {
  for (int i = 0; i < 4; i++) {
    bool off = pet.sleeping && i != 2;  // durmiendo solo funciona LUZ
    int bx = buttons[i].cx - BTN_HALF, by = buttons[i].cy - BTN_HALF;
    if (!pet.sleeping) gfx->fillRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, UI_WHITE);
    gfx->drawRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, inkColor());
    if (!off) drawMap(buttons[i].icon, 16, buttons[i].cx - 16, buttons[i].cy - 16, 2, false);
  }
}

const char *eggMsg() {
  switch (pet.eggCracks()) {
    case 0: return T(S_EGG_TOUCH);
    case 1: return T(S_EGG_MOVES);
    default: return T(S_EGG_ALMOST);
  }
}

const char *statusMsg() {
  if (pet.evolving()) return T(S_EVOLVING);
  if (bathUntil) return "Splish splash!";  // onomatopeya universal
  if (pet.sleeping) return "Zzz...";
  if (pet.eating()) return T(S_EATING);
  if (pet.showHeart()) return T(S_LIKES);
  if (pet.fullness < 25) return T(S_HUNGRY);
  if (pet.hygiene < 25) return T(S_NEEDS_BATH);
  if (pet.energy < 25) return T(S_EXHAUSTED);
  if (pet.joy < 25) return T(S_SAD);
  if (pet.weight > 60) return T(S_CHUBBY);
  if (pet.shiny && pet.ageMinutes < 15) return T(S_IS_SHINY);
  return T(S_HAPPY);
}

// dibuja un mapa de n x n pixeles escalado; silhouette=true lo pinta en tinta
void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette) {
  for (int r = 0; r < n; r++) {
    for (int c = 0; c < n; c++) {
      char ch = map[r][c];
      if (ch == '.') continue;
      gfx->fillRect(x + c * s, y + r * s, s, s, silhouette ? INK_K : spriteColor(ch));
    }
  }
}
