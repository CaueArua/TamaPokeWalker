# Tarefa: Refatoração do Sistema de Atributos (Stats, IVs, EVs, Nature) e Remodelação da Tela de Status

## Contexto do Projeto
Estou evoluindo um firmware de Tamagotchi/Pokéwalker em C++ (Arduino/ESP32-S3, tela redonda 466x466 com `Arduino_Canvas *gfx`).
Atualmente:
- O Pokémon ativo é gerenciado pela classe `Pet` (`pet.h` / `pet.cpp`, ambos na raiz do projeto, não em `src/`) e sincronizado com uma equipe de 6 slots (`PartySlot party[PARTY_SIZE]`).
- A ficha do Pokémon usa `cardPage` (definido em `src/ui/pet_card_page.cpp`, declarado em `src/ui/pet_card_page.h`), aberta por `openPetCardPage()` / `openPetCardSubpage()`. Não existe `cardOpen`, e `TamaPokeExtended.ino` não contém a lógica da ficha.
- A tela de status atual é `PetCardStatsScreen` (`src/ui/pet_card_stats_screen.cpp`), página `PET_CARD_PAGE_STATS` (6). As telas da ficha herdam de `PetCardPageScreen` (`src/ui/pet_card_page.h`), que já implementa `render()`, `handleTap()`, `handleSwipe()` e `handleSwipeV()` como `final`. Cada subclasse implementa `renderPage()` e, se precisar, `handlePageTap()`.
- Os atributos antigos usam `geneAtk/Def/Spe` e `trAtk/Def/Spe`. Os genes ficam em `pet.h` (`Pet`, ~linhas 146–147; `PartySlot`, ~linhas 125–126).

Quero substituir esse modelo pelo sistema oficial de **6 Status + Natureza + IVs + EVs** da Geração 3 em diante, tanto no `Pet` ativo quanto na `struct PartySlot`, e remodelar a tela de Status.

---

## 1. Alterações na Estrutura de Dados (`pet.h` e `pet.cpp`)

### A) Novos Campos no `Pet` e no `PartySlot`
Adicione (e sincronize em `Pet::syncToParty()` ~`pet.cpp:1847`, `Pet::switchPartySlot()` ~1875, `Pet::addToParty()` ~1915, `Pet::replacePartySlot()` ~1942 e `Pet::promoteNextFromParty()` ~163). Inclua os campos também no backup de save em `pet.cpp` (~344 e ~375):
- `uint8_t nature`: Índice de `0` a `24` representando as 25 Naturezas oficiais.
- `uint8_t ivs[6]`: Individual Values (`0` a `31`), estáticos por Pokémon, na ordem: `[0]=HP, [1]=Atk, [2]=Def, [3]=SpA, [4]=SpD, [5]=Spe`.
- `uint8_t evs[6]`: Effort Values (`0` a `252` por status, com teto global de **510** na soma dos 6 atributos).

### B) Inicialização e Compatibilidade com Saves Antigos
- Como os Pokémon já salvos na memória NVS (`prefs`) não possuem esses campos, garanta que a inicialização padrão (tanto para novos ovos/capturas quanto ao carregar um save antigo sem esses dados) defina:
  - **Natureza inicial:** Neutra (ex: `Hardy` ou `Serious`, multiplicador `1.0` em tudo).
  - **IVs iniciais:** `10` para todos os 6 status (`{10, 10, 10, 10, 10, 10}`).
  - **EVs iniciais:** `0` para todos os 6 status (`{0, 0, 0, 0, 0, 0}`).
- Crie um helper `addEv(uint8_t statIdx, uint8_t amount)` que respeite os limites máximos de `252` por atributo e `510` no total acumulado.
- Persistência: as chaves NVS atuais são `gatk`, `gdef`, `gspe`, `tatk`, `tdef` e `tspe` (gravação em `pet.cpp` ~1644–1647; leitura em ~1730–1738, onde `geneAtk == 0` regenera genes em saves legados). Use chaves novas com valor padrão em `prefs.getUChar(...)` / `prefs.getBool(...)` e não altere as antigas.

### C) Constante Temporária de Base Stats
- **Correção:** a tabela de base stats já existe. `DexEntry` em `dex.h` (linha 39) tem `bHp, bAtk, bDef, bSpe`, acessível por `DEX_TBL[speciesId]`. O arquivo é gerado, então não edite à mão. Não crie `DEFAULT_BASE_STAT`. Use `DEX_TBL[speciesId].bHp` para HP e `bAtk`/`bDef`/`bSpe` para os demais. **Atenção:** não existem `bSpA` nem `bSpD` (ver decisão 1 abaixo).

---

## 2. Fórmulas Oficiais de Cálculo (Geração 3+)

Implemente os métodos de cálculo usando **arredondamento para baixo (divisão inteira / `floor`) exatamente em cada etapa intermediária**, nunca apenas no final:

### A) Cálculo de HP (`statIdx == 0`)
- **Exceção Shedinja:** Se o Pokémon for o Shedinja (verifique por `DEX_TBL[speciesId].natNum == 292`, campo existente em `dex.h` linha 42), retorne sempre `1`.
- **Fórmula:**
  `HP = (((2 * base + iv + (ev / 4)) * nivel) / 100) + nivel + 10`
  *(Nota: `ev / 4` e `/ 100` devem ser divisões inteiras).*

### B) Demais Atributos (`Atk`, `Def`, `SpA`, `SpD`, `Spe`)
- **Fórmula:**
  1. `raw = (((2 * base + iv + (ev / 4)) * nivel) / 100) + 5` (usando divisão inteira).
  2. Multiplique `raw` pelo fator da Natureza (`1.1`, `1.0` ou `0.9`) e aplique `floor` (em aritmética inteira: `(raw * 11) / 10` para favorecido, `raw` para neutro, `(raw * 9) / 10` para prejudicado).

### C) Tabela Oficial das 25 Naturezas
Crie uma tabela estática com os nomes e multiplicadores (`0.9`, `1.0`, `1.1`). Nenhuma natureza altera `HP` (sempre `1.0`).

| Nature | Attack | Defense | Sp. Atk | Sp. Def | Speed |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Adamant** | 1.1 | 1.0 | 0.9 | 1.0 | 1.0 |
| **Bashful** | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| **Bold** | 0.9 | 1.1 | 1.0 | 1.0 | 1.0 |
| **Brave** | 1.1 | 1.0 | 1.0 | 1.0 | 0.9 |
| **Calm** | 0.9 | 1.0 | 1.0 | 1.1 | 1.0 |
| **Careful** | 1.0 | 1.0 | 0.9 | 1.1 | 1.0 |
| **Docile** | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| **Gentle** | 1.0 | 0.9 | 1.0 | 1.1 | 1.0 |
| **Hardy** | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| **Hasty** | 1.0 | 0.9 | 1.0 | 1.0 | 1.1 |
| **Impish** | 1.0 | 1.1 | 0.9 | 1.0 | 1.0 |
| **Jolly** | 1.0 | 1.0 | 0.9 | 1.0 | 1.1 |
| **Lax** | 1.0 | 1.1 | 1.0 | 0.9 | 1.0 |
| **Lonely** | 1.1 | 0.9 | 1.0 | 1.0 | 1.0 |
| **Mild** | 1.0 | 0.9 | 1.1 | 1.0 | 1.0 |
| **Modest** | 0.9 | 1.0 | 1.1 | 1.0 | 1.0 |
| **Naive** | 1.0 | 1.0 | 1.0 | 0.9 | 1.1 |
| **Naughty** | 1.1 | 1.0 | 1.0 | 0.9 | 1.0 |
| **Quiet** | 1.0 | 1.0 | 1.1 | 1.0 | 0.9 |
| **Quirky** | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| **Rash** | 1.0 | 1.0 | 1.1 | 0.9 | 1.0 |
| **Relaxed** | 1.0 | 1.1 | 1.0 | 1.0 | 0.9 |
| **Sassy** | 1.0 | 1.0 | 1.0 | 1.1 | 0.9 |
| **Serious** | 1.0 | 1.0 | 1.0 | 1.0 | 1.0 |
| **Timid** | 0.9 | 1.0 | 1.0 | 1.0 | 1.1 |

---

## 3. Remodelação da Tela de Status (`466x466` Redonda)

Remodele a tela de Status do Pokémon ativo (use a paleta das telas da ficha, incluída via `../../species.h` como em `src/ui/pet_card_stats_screen.cpp`: `UI_BG_DAY`, `UI_INK`, `UI_WHITE`, `UI_TRACK`, `UI_BAR_OK`, `UI_BAR_WARN`, `UI_BAR_BAD`) exibindo:
1. **Cabeçalho:** Nome/Espécie, Nível (`Lv.X`) e o nome da **Natureza** atual (ex: `Nature: Adamant`).
2. **Lista dos 6 Atributos (`HP`, `ATK`, `DEF`, `SP.A`, `SP.D`, `SPE`):**
   Cada linha deve conter exatamente nesta ordem horizontal:
   - **Nome do status** (destacando visualmente se é favorecido `+` ou prejudicado `-` pela Natureza).
   - **Valor Base + IV + Nível (com EV = 0):** O valor calculado pela fórmula oficial considerando `ev = 0`.
   - **Barra de progresso de EV:** Barra preenchida proporcionalmente de `0` até `252` (onde `100%` da barra = `252 EVs`).
   - **Valor Final Real:** O status final calculado considerando o `ev` atual do Pokémon.
3. **Rodapé — Barra de Afeição (`0` a `255`):**
   - Na parte inferior da tela, desenhe uma barra de progresso dedicada à **Afeição** (`0` a `255`), conectada diretamente ao campo `pet.bond` (que hoje representa o "Laço"), mostrando o valor numérico (`bond/255`) e o preenchimento proporcional.

## Entregáveis Esperados
1. As alterações completas para `pet.h` e `pet.cpp` (incluindo sincronização com `PartySlot`, tabela de Naturezas e fórmulas).
2. O código da nova tela de Status adaptada ao layout circular `466x466`, em `src/ui/status_screen.h` e `src/ui/status_screen.cpp`, herdando de `PetCardPageScreen` (não de `Screen`), implementando `renderPage()` e `handlePageTap()`. Para registrar a página, atualize as constantes em `src/ui/pet_card_page.h` (`PET_CARD_COUNT` é 9), `petCardScreenForPage()` em `src/ui/pet_card_page.cpp` (~linhas 36–46) e a regra de páginas de detalhe (`PET_CARD_SWIPE_COUNT`). Ou substitua a página `PET_CARD_PAGE_STATS` (ver decisão 3).
3. Ajuste chamadas antigas para não quebrar a compilação:
   - `atkStat()` / `defStat()` / `speStat()`: `pet.cpp` ~827–831, `src/ui/battle_screen.cpp` ~301–303, `src/serial_console.cpp` ~270–272 (debug) e `src/ui/pet_card_stats_screen.cpp` ~19–21.
   - `trAtk` / `trDef` / `trSpe` (se forem substituídos por EV): leitura em `src/ui/pet_card_expedition_screen.cpp` ~101 e escritas em `pet.cpp` ~1084, 1238, 1261, 1375, 1402 e 1572. Também `src/serial_console.cpp` ~201–208.
   - `calcStat()` é `static` em `pet.cpp` ~579. Substitua-o pelas novas fórmulas nesse mesmo arquivo.

## Decisões em aberto (confirmar antes de implementar)
1. **Atributos:** o jogo só tem Atk, Def e Spe. `battle.cpp` (~275) usa `bAtk` e `bSpe`, e `DexEntry` não tem `bSpA`/`bSpD`. Opções: (a) gerar `bSpA`/`bSpD` no gerador de `dex.h`; (b) mapear SpA = Atk e SpD = Def; (c) manter 3 atributos.
2. **`tr*` x EV:** os `trAtk/Def/Spe` atuais (0–100) são treino de expedição e treino, não EVs. Opções: (a) substituir `tr*` por `evs[]` com migração de valores e teto de 252; (b) manter `tr*` e criar `evs[]` separado.
3. **Tela:** substituir a página `PET_CARD_PAGE_STATS`, que hoje mostra ATK/DEF/SPE/WGT e V/D de batalha, ou criar uma página nova (`PET_CARD_COUNT` de 9 para 10).
4. **Batalha selvagem:** `battle.cpp` (~275) calcula `bAtk + lvl`, sem `atkStat()`. Decida se a fórmula nova também vale para batalhas selvagens.
