#pragma once
#include <FS.h>

// Centraliza toda a configuração de armazenamento da placa em um único arquivo
bool initBoardStorage();
fs::FS& getBoardStorage();