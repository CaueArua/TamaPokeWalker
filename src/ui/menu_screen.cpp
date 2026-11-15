#include "menu_screen.h"
#include <Arduino_GFX_Library.h>
#include "../../species.h"
#include "../../audio.h"
#include "dex_screen.h"
#include "party_screen.h"
#include "screen_controller.h"

extern Arduino_Canvas *gfx;
extern void flushStampedFrame();

static constexpr int M_CX = 233;

// Cores locais complementares
static constexpr uint16_t M_DISABLED_BG  = 0xE73D; // Cinza claro para botões desabilitados
static constexpr uint16_t M_DISABLED_INK = 0x8410; // Cinza escuro para ícone/texto desabilitado
static constexpr uint16_t M_POKE_RED     = 0xEA87;
static constexpr uint16_t M_BAG_BROWN    = 0xB3C8;
static constexpr uint16_t M_GOLD         = 0xFECB;

// Coordenadas da grade 2x2 centralizada na tela redonda 466x466
static constexpr int BTN_W     = 148;
static constexpr int BTN_H     = 128;
static constexpr int BTN_GAP_X = 16;
static constexpr int BTN_GAP_Y = 16;
static constexpr int GRID_X0   = M_CX - BTN_W - (BTN_GAP_X / 2); // 77
static constexpr int GRID_Y0   = 96;

MenuScreen menuScreen;

// =========================================================================
// DESENHO DOS ÍCONES VETORIAIS DE CADA OPÇÃO
// =========================================================================

// 1. Ícone PARTY: 3 Pokébolas representando a equipe
static void drawIconParty(int cx, int cy) {
  const int offsets[] = { -24, 0, 24 };
  const int r = 10;
  for (int i = 0; i < 3; i++) {
    int bx = cx + offsets[i];
    int by = (i == 1) ? cy - 4 : cy + 2; // Central levemente elevada
    // Metade superior vermelha e inferior branca
    gfx->fillCircle(bx, by, r, M_POKE_RED);
    gfx->fillRect(bx - r, by, r * 2 + 1, r + 1, UI_WHITE);
    // Recorte redondo e contorno
    gfx->drawCircle(bx, by, r, UI_INK);
    gfx->drawFastHLine(bx - r + 1, by, r * 2 - 1, UI_INK);
    gfx->fillCircle(bx, by, 3, UI_INK);
    gfx->fillCircle(bx, by, 1, UI_WHITE);
  }
}

// 2. Ícone BOX: Caixa de armazenamento PC (desabilitada em cinza)
static void drawIconBox(int cx, int cy, uint16_t color) {
  // Corpo da caixa
  gfx->drawRoundRect(cx - 24, cy - 12, 48, 30, 5, color);
  gfx->drawRoundRect(cx - 23, cy - 11, 46, 28, 4, color);
  // Tampa superior
  gfx->fillRoundRect(cx - 27, cy - 18, 54, 9, 3, color);
  // Trava central
  gfx->fillRoundRect(cx - 6, cy - 6, 12, 10, 2, color);
}

// 3. Ícone DEX: Pokédex aberta
static void drawIconDex(int cx, int cy, uint16_t color) {
  // Capa esquerda e direita
  gfx->drawRoundRect(cx - 24, cy - 18, 23, 36, 4, color);
  gfx->drawRoundRect(cx + 1, cy - 18, 23, 36, 4, color);
  // Lente principal no topo esquerdo
  gfx->fillCircle(cx - 16, cy - 11, 4, color);
  // Tela interna esquerda
  gfx->fillRect(cx - 20, cy - 3, 15, 13, color);
  // Mini teclado direito
  gfx->fillRect(cx + 5, cy - 10, 15, 8, color);
  gfx->drawFastHLine(cx + 5, cy + 4, 15, color);
  gfx->drawFastHLine(cx + 5, cy + 9, 15, color);
}

// 4. Ícone BAG: Mochila de treinador
static void drawIconBag(int cx, int cy, bool enabled) {
  uint16_t outline = enabled ? UI_INK : M_DISABLED_INK;
  uint16_t body = enabled ? M_BAG_BROWN : M_DISABLED_BG;
  uint16_t flap = enabled ? M_POKE_RED : M_DISABLED_BG;
  uint16_t buckle = enabled ? M_GOLD : M_DISABLED_INK;
  // Alça superior
  gfx->drawRoundRect(cx - 10, cy - 22, 20, 10, 4, outline);
  // Corpo principal da mochila
  gfx->fillRoundRect(cx - 22, cy - 14, 44, 32, 9, body);
  gfx->drawRoundRect(cx - 22, cy - 14, 44, 32, 9, outline);
  // Aba frontal
  gfx->fillRoundRect(cx - 22, cy - 14, 44, 15, 8, flap);
  gfx->drawRoundRect(cx - 22, cy - 14, 44, 15, 8, outline);
  // Fivela dourada central
  gfx->fillRoundRect(cx - 5, cy - 3, 10, 8, 2, buckle);
  gfx->drawRoundRect(cx - 5, cy - 3, 10, 8, 2, outline);
}

// =========================================================================
// RENDERIZAÇÃO E EVENTOS DA TELA
// =========================================================================

void MenuScreen::render() {
  dirty = false;
  gfx->fillScreen(UI_BG_DAY);

  // Cabeçalho
  const char *title = "MENU";
  gfx->setTextColor(UI_INK);
  gfx->setTextSize(3);
  gfx->setCursor(M_CX - (int)strlen(title) * 9, 42);
  gfx->print(title);

  struct MenuItem {
    const char *label;
    bool enabled;
  };

  const MenuItem items[] = {
    { "PARTY", true  },
    { "BOX",   false },
    { "DEX",   true  },
    { "BAG",   false }
  };

  for (uint8_t i = 0; i < 4; i++) {
    int col = i % 2;
    int row = i / 2;
    int x = GRID_X0 + col * (BTN_W + BTN_GAP_X);
    int y = GRID_Y0 + row * (BTN_H + BTN_GAP_Y);

    bool enabled = items[i].enabled;
    uint16_t bgCol     = enabled ? UI_WHITE : M_DISABLED_BG;
    uint16_t borderCol = enabled ? UI_INK   : UI_TRACK;
    uint16_t textCol   = enabled ? UI_INK   : M_DISABLED_INK;

    // Cartão do botão
    gfx->fillRoundRect(x, y, BTN_W, BTN_H, 16, bgCol);
    gfx->drawRoundRect(x, y, BTN_W, BTN_H, 16, borderCol);
    if (enabled) {
      gfx->drawRoundRect(x + 1, y + 1, BTN_W - 2, BTN_H - 2, 15, borderCol);
    }

    // Desenha o símbolo correspondente na metade superior do botão
    int iconCx = x + BTN_W / 2;
    int iconCy = y + 48;
    if (i == 0)      drawIconParty(iconCx, iconCy);
    else if (i == 1) drawIconBox(iconCx, iconCy, M_DISABLED_INK);
    else if (i == 2) drawIconDex(iconCx, iconCy, textCol);
    else if (i == 3) drawIconBag(iconCx, iconCy, enabled);

    // Rótulo centralizado na parte inferior do botão
    int labelW = (int)strlen(items[i].label) * 12; // textSize 2 = 12px por caractere
    gfx->setTextSize(2);
    gfx->setTextColor(textCol);
    gfx->setCursor(x + (BTN_W - labelW) / 2, y + 92);
    gfx->print(items[i].label);
  }

  // Rodapé de retorno
  gfx->setTextSize(1);
  gfx->setTextColor(UI_TRACK);
  gfx->setCursor(M_CX - 57, 396);
  gfx->print("< DESLIZE P/ VOLTAR");

  flushStampedFrame();
}

static int8_t menuItemAt(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 4; i++) {
    int col = i % 2;
    int row = i / 2;
    int itemX = GRID_X0 + col * (BTN_W + BTN_GAP_X);
    int itemY = GRID_Y0 + row * (BTN_H + BTN_GAP_Y);
    if (x >= itemX && x < itemX + BTN_W &&
        y >= itemY && y < itemY + BTN_H) return (int8_t)i;
  }
  return -1;
}

void MenuScreen::handleTap(int16_t x, int16_t y) {
  if (y >= 385) {
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
    return;
  }

  switch (menuItemAt(x, y)) {
    case 0:
      partyScreen.open(&menuScreen);
      sfxPlay(SFX_MENU);
      break;
    case 2:
      dexScreen.openGrid(&menuScreen);
      setScreen(&dexScreen);
      sfxPlay(SFX_MENU);
      break;
    default:
      break;
  }
}

void MenuScreen::handleSwipe(int direction) {
  if(direction == 1){
    sfxPlay(SFX_TAP);
    setScreen(nullptr);
  }  
}
