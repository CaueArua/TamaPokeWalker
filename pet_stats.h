#pragma once
#include <cmath>
#include <cstdint>

// Calcula o HP final do Pokémon
inline uint16_t calcularHP(uint8_t base, uint8_t iv, uint8_t ev, uint8_t nivel, bool isShedinja = false) {
    if (isShedinja) {
        return 1;
    }
    return static_cast<uint16_t>(((2 * base + iv + (ev / 4)) * nivel) / 100 + nivel + 10);
}

// Calcula Attack, Defense, Sp. Atk, Sp. Def e Speed
// O parâmetro 'nature' deve ser 1.1 (positiva), 0.9 (negativa) ou 1.0 (neutra)
inline uint16_t calcularStat(uint8_t base, uint8_t iv, uint8_t ev, uint8_t nivel, double nature = 1.0) {
    uint16_t valorBase = ((2 * base + iv + (ev / 4)) * nivel) / 100 + 5;
    return static_cast<uint16_t>(std::floor(valorBase * nature));
}
