#include "game_info_screen.h"

#include <Arduino_GFX_Library.h>
#include <stdio.h>
#include <string.h>

#include "../../audio.h"
#include "../../biome.h"
#include "../../game_info.h"
#include "../../i18n.h"
#include "../../species.h"

extern Arduino_Canvas *gfx;
extern void flushStampedFrame();

GameInfoScreen gameInfoScreen;

struct GameInfoLabels {
  const char *title;
  const char *player;
  const char *id;
  const char *region;
  const char *route;
  const char *biome;
};

static const GameInfoLabels LABELS[LANG_COUNT] = {
  { "DATOS", "JUGADOR", "ID", "REGION", "RUTA", "BIOMA" },
  { "GAME INFO", "PLAYER", "ID", "REGION", "ROUTE", "BIOME" },
  { "INFOS", "JOUEUR", "ID", "REGION", "ROUTE", "BIOME" },
  { "INFOS", "SPIELER", "ID", "REGION", "ROUTE", "BIOM" },
  { "INFO", "GIOCATORE", "ID", "REGIONE", "ROTTA", "BIOMA" },
  { "DADOS", "JOGADOR", "ID", "REGIAO", "ROTA", "BIOMA" },
};

static const char *const BIOME_NAMES[LANG_COUNT][BIOME_COUNT] = {
  { "PRADERA", "PLAYA", "BOSQUE", "VOLCAN", "MONTANA", "NIEVE", "CIUDAD", "PANTANO", "CUEVA", "FONDO MARINO", "ALTA MAR" },
  { "GRASSLAND", "BEACH", "FOREST", "VOLCANO", "MOUNTAIN", "SNOW", "CITY", "SWAMP", "CAVE", "SEABED", "OPEN SEA" },
  { "PRAIRIE", "PLAGE", "FORET", "VOLCAN", "MONTAGNE", "NEIGE", "VILLE", "MARAIS", "GROTTE", "FOND MARIN", "HAUTE MER" },
  { "GRASLAND", "STRAND", "WALD", "VULKAN", "BERG", "SCHNEE", "STADT", "SUMPF", "HOEHLE", "MEERESBODEN", "HOHE SEE" },
  { "PRATERIA", "SPIAGGIA", "FORESTA", "VULCANO", "MONTAGNA", "NEVE", "CITTA", "PALUDE", "GROTTA", "FONDALE", "ALTO MARE" },
  { "CAMPO", "PRAIA", "FLORESTA", "VULCAO", "MONTANHA", "NEVE", "CIDADE", "PANTANO", "CAVERNA", "FUNDO DO MAR", "ALTO MAR" },
};

static const GameInfoLabels &labels() {
  return LABELS[gLang < LANG_COUNT ? (uint8_t)gLang : (uint8_t)LANG_EN];
}

static void centered(const char *text, int x, int y, int size, uint16_t color) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  gfx->setCursor(x - (int)strlen(text) * 3 * size, y);
  gfx->print(text);
}

static void drawRow(int y, const char *label, const char *value) {
  centered(label, 233, y, 1, UI_TRACK);
  centered(value[0] ? value : "-", 233, y + 18, 2, UI_INK);
}

static constexpr int16_t CLOSE_TAP_Y = 380;

void GameInfoScreen::open(Screen *returnTo) {
  returnScreen = returnTo;
  dirty = true;
  setScreen(this);
  sfxPlay(SFX_MENU);
}

void GameInfoScreen::render() {
  dirty = false;
  const GameInfoLabels &l = labels();
  uint8_t lang = gLang < LANG_COUNT ? (uint8_t)gLang : (uint8_t)LANG_EN;
  gfx->fillScreen(UI_BG_DAY);
  centered(l.title, 233, 42, 3, UI_INK);

  char idText[8];
  snprintf(idText, sizeof(idText), "%06u", (unsigned)gameInfo.playerId);
  const char *biomeText = gameInfo.biome < BIOME_COUNT ? BIOME_NAMES[lang][gameInfo.biome] : "";

  drawRow(92, l.player, gameInfo.playerName);
  drawRow(160, l.id, idText);
  drawRow(228, l.region, gameInfo.region);
  drawRow(296, l.route, gameInfo.route);
  drawRow(364, l.biome, biomeText);
  centered("TAP BOTTOM TO RETURN", 233, 420, 1, UI_TRACK);
  flushStampedFrame();
}

void GameInfoScreen::handleSwipeV(int) {
  close();
}

void GameInfoScreen::handleTap(int16_t, int16_t y) {
  if (y >= CLOSE_TAP_Y) close();
}

void GameInfoScreen::close() {
  Screen *target = returnScreen;
  returnScreen = nullptr;
  setScreen(target);
  sfxPlay(SFX_TAP);
}

bool GameInfoScreen::isStaticClean() const {
  return !dirty;
}

void GameInfoScreen::markDirty() {
  dirty = true;
}
