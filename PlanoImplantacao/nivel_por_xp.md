# Plano de implantação: nível por XP

## 1. Objetivo

Remover o cálculo de nível baseado em tempo de vida (`ageMinutes / MINUTES_PER_LEVEL`) e passar a derivar o nível do XP acumulado.

Requisitos:

- Variável `level` com o nível atual do Pokémon.
- Método `Pet::addXp(X)` que soma XP, verifica se houve subida de nível e atualiza `level`.
- Ao atingir L100, o Pokémon para de ganhar XP e o XP total fica exatamente igual a `getTotalExp(grupo, 100)`.
- Grupo de crescimento de cada espécie vem da National Dex.
- Fonte de XP inicial: 1 XP a cada 15 segundos, somente enquanto acordado.
- Offline não concede XP.
- Saves existentes são migrados convertendo o nível atual (`ageMinutes`) em XP.

Fora de escopo:

- Novas fontes de XP além do tempo acordado (batalhas, cuidados, passos etc.), que serão tratadas depois.
- Mudanças em regras de evolução.

## 2. Situação atual

| Ponto | Situação |
|---|---|
| Nível | `Pet::level()` = `1 + ageMinutes / MINUTES_PER_LEVEL` em [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h:477) |
| Idade | `ageMinutes` também é usado como idade real: ficha (dias), farewell, adiamento de evolução, chocar ovo, medalhas |
| XP | `xp` existe em `Pet` e `PartySlot`, é salvo, carregado e copiado, mas nenhum código o incrementa |
| Curva de XP | `getTotalExp(GrowthRate, level)` em [pet_growth.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet_growth.h:8), com 6 grupos |
| Grupo por espécie | `nationalDexGetGrowthRate(id, &out)` em [nationalDex.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/nationalDex.h:65), retorna string (ex.: `"medium-fast"`). Padrão quando ausente: `"medium-fast"` ([nationalDex.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/nationalDex.cpp:19)) |
| Teto de XP | `PET_XP_MAX = 1640000` ([pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h:144)) coincide só com o L100 do grupo Fluctuating. Deve ser substituído pelo teto do grupo da espécie |
| Erro de build | [Erros.txt](C:/Users/cauea/Documents/projetos/TamaPokeExtended/Erros.txt): `'pwrSetup' was not declared` em [program_setup.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/program_setup.cpp:29). Pré-existente, bloqueia a compilação |

Referências de ritmo atual: hoje o nível sobe 1 por hora, então L100 leva cerca de 99 horas de uso ativo. Com 1 XP a cada 15 s (4 XP/min), o MediumFast (1.000.000 XP) leva cerca de 250.000 min, ou seja, cerca de 174 dias de uso ativo.

## 3. Arquitetura proposta

### 3.1 Funções puras (`pet_growth.h`)

- `GrowthRate growthRateFromName(const char *name)`: converte `"slow"`, `"medium-slow"`, `"medium-fast"`, `"fast"`, `"erratic"`, `"fluctuating"`. Desconhecido ou ausente retorna `MediumFast`.
- `int levelFromXp(GrowthRate group, long long xp)`: maior L (1..100) com `getTotalExp(group, L) <= xp`.
- `long long xpCapFor(GrowthRate group)`: `getTotalExp(group, 100)`.

### 3.2 Estado do `Pet`

- Novo membro `uint8_t level` (em memória, sempre consistente com `xp`).
- Grupo obtido pela Dex a partir de `speciesId` (não é salvo, por isso não altera o formato do save).
- `Pet::addXp(uint32_t amount)`:
  - Se `isEgg()` ou `level >= 100`, não faz nada e retorna 0.
  - Soma `amount`, limitando a `xpCapFor(group)`. Ao chegar em L100, `xp` recebe exatamente o valor de `getTotalExp(group, 100)`.
  - Recalcula `level` com `levelFromXp`.
  - Retorna a quantidade de níveis ganhos. Se subir, toca `SFX_LEVEL`.
- `level()` passa a retornar `level`.
- `ageMinutes` continua existindo apenas como idade, sem efeito no nível.
- Remover `ageMinutes % MINUTES_PER_LEVEL == 0` do tick ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:266)). O SFX passa a sair de `addXp`.

### 3.3 Ganho de XP por tempo acordado

- Acumulador de 15 s (padrão do `TickScheduler`), chamado em `Pet::update`.
- Concede 1 XP por intervalo somente se: não é ovo, não está dormindo e não há cerimônia ativa.
- `applyOfflineMinutes` não concede XP.

### 3.4 Party

- `PartySlot` já tem `xp`. O nível do slot é derivado de `xp` ao carregar ou trocar de slot. Nenhuma mudança de layout.
- `addToParty`, `replacePartySlot` e `setActiveLevel` ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:2014)) passam a definir `xp = getTotalExp(group, lv)` em vez de `ageMinutes`.
- `syncToParty` e `switchPartySlot` continuam copiando `xp`. `promoteNextFromParty` e `switchPartySlot` recalculam `level` ao carregar um slot.
- [party_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/party_screen.cpp:51) passa a usar `levelFromXp(growth, slot.xp)`.

### 3.5 Migração (save antigo)

- Flag NVS `"xpv"`. Se ausente, na primeira carga:
  - Ativo: `xp = getTotalExp(group, 1 + ageMinutes / 60)`.
  - Cada slot da party: mesma conversão com o `ageMinutes` do slot.
  - Blobs legados `PartySlotV2` e `PreXp` passam pela mesma conversão ao serem lidos.
- Depois de gravar, marcar `"xpv"` como concluído.

### 3.6 Pontos a ajustar

| Local | Mudança |
|---|---|
| [pet_card_progress_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_progress_screen.cpp:25) | Barra: `(xp - getTotalExp(L)) / (getTotalExp(L+1) - getTotalExp(L))`, no lugar de `ageMinutes % 60` |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:108) (`AGE`) | Ajustar idade sem alterar nível, ou ajustar XP se for ferramenta de nível |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:206) (`TESTEVO`) | Definir `xp` para atingir o nível exigido, no lugar de `ageMinutes` |
| `setActiveLevel` ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:2140)) | Definir `xp = getTotalExp(group, lv)` |
| Evolução | Sem mudança de regra. `evolutionRequiredLevelFor` continua usando `level()` |

## 4. Fases de implantação

Cada fase deve compilar e manter o comportamento. Um commit por fase.

| Fase | Entrega | Critério de aceite |
|---|---|---|
| 0 | Corrigir `'pwrSetup' was not declared` em `program_setup.cpp` | Firmware compila |
| 1 | `growthRateFromName`, `levelFromXp`, `xpCapFor` em `pet_growth.h` com testes | Testes nativos passam: limites de nível, L100 exato, XP acima do teto, cada grupo |
| 2 | `level` e `addXp` em `Pet`; `level()` retorna a variável; SFX movido | Compila; testes de `addXp` (subida, teto L100) |
| 3 | Acumulador de 15 s em `update` | 1 XP a cada 15 s acordado; zero em ovo, sono, cerimônia e offline |
| 4 | Party: `addToParty`, `replacePartySlot`, `setActiveLevel`, `switchPartySlot`, `promoteNextFromParty`, `party_screen.cpp` | Nível do slot correto ao trocar, adicionar e substituir |
| 5 | Migração `"xpv"` para ativo e party, incluindo blobs legados | Save antigo mantém o nível atual após reboot |
| 6 | Barra de progresso, `TESTEVO`, `LEVEL`, `AGE` | Telas e comandos seriais coerentes com XP |
| 7 | Atualizar documentação (`ARQUITETURA.md`, se existir) | Documentação reflete nível por XP |

## 5. Testes

### 5.1 Nativos

- `levelFromXp` nos limites de cada nível, em cada grupo.
- `xpCapFor` igual a `getTotalExp(group, 100)` para cada grupo.
- `addXp` acima do teto: XP fica exatamente no teto e o nível é 100.
- `addXp` em L100: retorna 0 e não altera XP.
- Migração: `getTotalExp(group, 1 + ageMinutes/60)` produz o nível esperado.

Não testar constantes nem acessores triviais.

### 5.2 Checklist de hardware

- [ ] Pokémon acordado ganha 1 XP a cada 15 s.
- [ ] Dormindo, em ovo ou com cerimônia ativa: XP não aumenta.
- [ ] Ao subir de nível, toca o SFX e a barra reinicia.
- [ ] Em L100, XP para e o valor é exatamente o do teto do grupo.
- [ ] Reiniciar: nível e XP persistem.
- [ ] Save antigo: nível atual preservado após a migração.
- [ ] Trocar de slot na party: nível de cada membro está correto.

## 6. Riscos

| Risco | Mitigação |
|---|---|
| Migração errada faz Pokémon cair de nível | Conversão com `getTotalExp` do nível atual e flag `"xpv"` gravada só após sucesso |
| Ritmo muito lento (174 dias para L100 no MediumFast) | Confirmar o ritmo antes de implantar. Ajustar a taxa de 15 s é uma constante |
| Nível da party desalinhado com XP | Derivar `level` sempre por `levelFromXp` ao carregar ou trocar slot |
| Espécie sem grupo na Dex | Fallback para `MediumFast` |
| Build quebrado pelo erro de `pwrSetup` | Fase 0 antes de qualquer outra mudança |

## 7. Fora de escopo (futuro)

- Novas fontes de XP: batalhas, cuidados, passos, minigames.
- XP offline.
- Mudanças nas regras de evolução por nível.
