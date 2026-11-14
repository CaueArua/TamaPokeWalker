#pragma once
#include <Arduino.h>
#include <stdint.h>

class Screen {
public:
  virtual ~Screen() {}

  
  virtual void onEnter() {} // Chamado uma vez quando a tela é aberta
  virtual void onExit() {} // Chamado uma vez quando a tela é fechada

  // Desenha a tela (deve chamar flushStampedFrame() no final)
  virtual void render() = 0;

  // Eventos de toque e deslize
  virtual void handleTap(int16_t x, int16_t y) {}
  virtual void handlePress(int16_t y) {}
  virtual uint32_t longPressDelayMs() const { return 700; } // Tempo padrão (customizável por tela)
  virtual void handleLongPress(int16_t x, int16_t y, uint32_t durationMs) {}
  virtual void handleSwipe(int dir) {}    // -1 = Esquerda, +1 = Direita
  virtual void handleSwipeV(int dir) {}   // -1 = Cima, +1 = Baixo

  // Opcional: retorna true se a tela é estática e não precisa redesenhar neste frame
  virtual bool isStaticClean() const { return false; }
  virtual void markDirty() {}
};

// Ponteiro global para a tela ativa (nullptr = tela principal do Tamagotchi)
extern Screen* activeScreen;

// Função auxiliar para trocar de tela de forma segura
void setScreen(Screen* next);
