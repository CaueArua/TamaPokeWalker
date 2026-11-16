#include "board_storage.h"
#include <FFat.h> // Se mudar de placa no futuro, troca só este include!

bool initBoardStorage() {
  return FFat.begin(true);
}

fs::FS& getBoardStorage() {
  return FFat;
}