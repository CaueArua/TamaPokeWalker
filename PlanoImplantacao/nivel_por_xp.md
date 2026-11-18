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
- Tela de progresso mostra a barra com base no XP e o XP necessário para o próximo nível.
- Evolução por nível passa a comparar XP com o limiar do grupo (mesmas regras de nível mínimo e cuidados).
- Todo local que usa nível para decidir ações passa a ser derivado do XP.
- Remover o mecanismo de "despedida" (farewell), que hoje é oferecido ao atingir idade e/ou nível máximo.

Fora de escopo:

- Novas fontes de XP além do tempo acordado (batalhas, cuidados, passos etc.), que serão tratadas depois.
- Alterar os requisitos de nível das regras de evolução (apenas a forma de checar muda).

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

- Acumulador de 15 s medido com `millis()`, chamado em `Pet::update`. Não usar `lastSeenEpoch`: ajustes de relógio (`Pet::setClock`, `Clock::set`) não devem alterar o XP.
- Concede 1 XP por intervalo somente se: não é ovo, não está dormindo e não há cerimônia ativa.
- `applyOfflineMinutes` não concede XP. Ele continua incrementando só `ageMinutes` e os atributos.

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

### 3.6 Relógio e fluxo de boot

- `Clock::begin()` (em `sensor_setup.cpp`) carrega a base do relógio por software.
- `initializeProgram` ([program_setup.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/program_setup.cpp:31)):
  - `Clock::nowEpoch() == 0`: abre a tela de ajuste de hora e **não** chama `syncClock`.
  - Caso contrário: `pet.syncClock(e)`, que usa a flag NVS `"seen"` e aplica `applyOfflineMinutes` quando passaram 2 min ou mais (limite de 14 dias).
- Consequência para XP: o XP offline continua zero, então a mudança de nível não depende do relógio.
- Consequência para o relógio por software: ele só avança com `millis()` enquanto ligado. Sem RTC, o tempo desligado não é contado, porque a base salva a cada 60 s (`Clock::tick`) é restaurada sem avanço. O `ageMinutes` offline pode ficar subcontado nesse caso. Isso não afeta o nível novo.
- A migração (3.5) usa `ageMinutes` do save, não o relógio. Portanto ela não depende de `"seen"`.
- `Pet::setClock` continua usado pelos comandos seriais `TIME` e `RTCSET`. Não é alterado por este plano.

### 3.7 Pontos a ajustar

| Local | Mudança |
|---|---|
| [pet_card_progress_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_progress_screen.cpp:25) | Barra: `(xp - getTotalExp(L)) / (getTotalExp(L+1) - getTotalExp(L))`, no lugar de `ageMinutes % 60` |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:108) (`AGE`) | Ajustar idade sem alterar nível, ou ajustar XP se for ferramenta de nível |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:206) (`TESTEVO`) | Definir `xp` para atingir o nível exigido, no lugar de `ageMinutes` |
| `setActiveLevel` ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:2140)) | Definir `xp = getTotalExp(group, lv)` |
| Evolução | Sem mudança de regra. `evolutionRequiredLevelFor` continua usando `level()` |

### 3.8 Tela de progresso (item a)

Arquivo: [pet_card_progress_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_progress_screen.cpp).

- Grupo de crescimento da espécie ativa, usado para as duas curvas.
- Barra: `(xp - getTotalExp(g, L)) / (getTotalExp(g, L+1) - getTotalExp(g, L))`, no lugar de `ageMinutes % 60`.
- Texto abaixo da barra: `xp_atual / xp_do_proximo_nivel` (ex.: `12345 / 20000 XP`), com a string `S_NEXT_LVL_FMT` em `i18n.cpp`, nas 6 línguas.
- Em L100 a barra fica cheia e o texto mostra o XP do teto (sem próximo nível).
- Helper novo em `pet_growth.h`: `long long xpForLevel(GrowthRate g, uint8_t L)` (wrapper de `getTotalExp`), para não repetir a conversão nas telas.

### 3.9 Evolução por XP (item b)

Hoje a regra compara `pet.level()` com `minLevel` em `evolutionRuleReady` (≈ [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:915)). Pontos afetados: `evolutionRequiredLevel`, `evolutionRequiredLevelFor`, `evolutionUnlocked`, `evolutionLevelReached`, `evolutionNeedsMoreBond`, `wantEvolveButton`, `declineEvolve`.

Mudança:

- Limiar em XP: `xpNeeded = getTotalExp(group, minLevel + careMistakes)`. A regra fica `xp >= xpNeeded`.
- `level()` continua existindo para exibição, mas nenhuma decisão de evolução o usa diretamente.
- Textos "Requer nível N" continuam mostrando o nível (derivado de `levelFromXp`).
- Recusa permanente (`declineEvolve`, hoje `evoDeclinedLv` e `evoDeclinedAge`, que adiam a oferta por nível ou por tempo):
  - Remover `evoDeclinedLv`, `evoDeclinedAge`, `resetEvolutionDeferral` e a regra de prazo em `wantEvolveButton`. Não existe mais adiamento por nível, por XP ou por tempo.
  - Novo campo `evoDismissed` (bool) por indivíduo. Ao tocar em "manter forma", `declineEvolve()` grava `evoDismissed = true`.
  - Com `evoDismissed == true`, o diálogo de evolução não abre automaticamente na tela principal, e o botão pulsante da tela principal também não aparece para aquele pet. A recusa vale só para o pet que recusou.
  - O campo fica no `Pet` ativo e também em `PartySlot`. Ao trocar para outro membro da party, o flag vem junto com o membro. Se o membro estiver apto e não tiver recusado, o diálogo aparece normalmente.
  - Persistência: NVS com chave nova (ex.: `"evod"`), gravada em `save()`. Ao criar ou liberar um pet, o flag volta a `false`. Ao chocar um ovo, volta a `false`. Ao evoluir, o flag volta a `false`.
  - Backup: `PetBackup` hoje carrega `evoDeclinedLv` e `evoDeclinedAge`. Trocar esses dois campos por `evoDismissed` muda o layout da struct. Antes de implantar, verificar como `importBackup` valida a versão. Se o layout mudar, subir a versão e decidir como aceitar backups antigos (ler o flag como `false`).
  - O botão da tela de progresso (3.12) não depende do flag. Ele aparece sempre que `canEvolveNow()` for verdadeiro, e o toque abre o diálogo mesmo depois de uma recusa. Esse toque é ação explícita do usuário.
- Migração de save antigo: `evoDeclinedLv` e `evoDeclinedAge` são lidos uma vez e descartados. `evoDismissed` começa como `false`. Essa migração fica sob a flag `"xpv"`.
- Decisão do usuário: a recusa é zerada ao evoluir. Assim, a nova forma pode oferecer a evolução seguinte normalmente. Dentro da mesma forma, a recusa é permanente.

### 3.10 Outros pontos que usam nível para decidir ações (item c)

| Local | Uso atual | Mudança |
|---|---|---|
| [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:563) (`checkMedals`) | Medalhas em níveis 5 (Fit), 10, 25, 50 | Comparar `level` derivado do XP (já coerente se `level` é atualizado em `addXp`). Medalhas são concedidas no evento de subida |
| [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:915) (evolução) | `level() < need` | Item 3.9 |
| [battle_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/battle_screen.cpp:300) (linhas 300, 341, 346) | `wildLevelFor(pet.level(), ...)` | Sem mudança de código: `level()` passa a vir do XP |
| [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp:859) | Nível do prompt de batalha selvagem | Sem mudança de código |
| [party_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/party_screen.cpp:51) | `1 + slot.ageMinutes / 60` | `levelFromXp(group, slot.xp)` |
| `setActiveLevel` ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:2140)) e setters em ≈2022, 2045 | Derivam `ageMinutes` a partir do nível | Derivam `xp = getTotalExp(group, lv)` |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:107) `LVL n` | `ageMinutes = n * 60`, sem alterar o nível (o nível vinha da idade) | Passa a chamar `setActiveLevel(n)`, igual ao `LEVEL n`. Grava XP |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:205) `TESTEVO` | Ajusta `ageMinutes` | Ajusta `xp` para o nível exigido |
| [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:343, 397, 414, 429) | Imprime nível | Sem mudança de lógica |
| Telas de resumo, perfil e stats | Mostram nível | Sem mudança de lógica |
| Shiny e cerimônia de despedida ([pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:432)) | Usa `ageMinutes` / despedida | Ver 3.11 |

Sem pontos em aberto nesta seção.

### 3.11 Remoção da despedida (item d)

Decisão do usuário: remover o mecanismo de despedida ao atingir idade e/ou nível máximo, incluindo o bônus de shiny ligado a ele.

Escopo atual da despedida (fonte: [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:844)):

- `canFarewellNow()`: não é ovo, não dorme, sem cerimônia, forma final (sem caminho de evolução) e `ageMinutes >= FAREWELL_AGE_MIN` (3 dias).
- Hoje, L100 não dispara a despedida. A remoção cobre os dois gatilhos pedidos (idade e nível máximo), para que nenhum deles inicie a despedida.

Remover:

- [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h:17) `FAREWELL_AGE_MIN`; [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h:21) `CER_FAREWELL`; `canFarewellNow`, `startFarewell`, `declineFarewell`, `wantFarewellButton` (≈409–466); `farDeclinedAge` (≈588).
- [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:855) `startFarewell`; [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:432) bônus de shiny da despedida (24 vs 48); [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:1943) restauração de cerimônia pendente.
- [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp:128) botão, [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp:580) desenho da cerimônia, [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp:1080) diálogo de decisão, e referências em 207, 434, 488, 1547, 1597.
- [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp:157) comando `FAREWELL`.
- Nada em [audio.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/audio.h:17): `SFX_BYE` continua em uso pela fuga (`startRunaway`).
- `i18n.h` e `i18n.cpp`: `S_FAREWELL`, `S_FAREWELL_BTN` e as strings do diálogo de despedida, nas 6 línguas.

Manter: fuga (`CER_RUNAWAY`) e liberação (`CER_RELEASE`), que não dependem de idade ou nível.

Migração de save:

- Se o save estiver em `pendingCeremony == CER_FAREWELL` (valor de `pet.h:21`), tratar como `CER_NONE` e gerar um ovo, como já é feito para outros valores de cerimônia pendente em [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp:1943). Manter o valor numérico de `CER_RUNAWAY` e `CER_RELEASE` inalterado para não quebrar saves existentes. Para isso, `CER_FAREWELL` continua no enum como valor reservado (`= 1`, sem uso). Na carga, `lend` e `cerp` iguais a `CER_FAREWELL` viram `CER_NONE`. O mesmo mapeamento vale em `importBackup`.

Riscos da remoção:

- `lend` (NVS `"lend"`) e `cerp` (NVS `"cerp"`) guardam o valor numérico de `CER_*`, e o mesmo valor vai no backup (`PetBackup.lastEnd`). Se o enum fosse renumerado, um save ou backup antigo com valor 1 (fuga) passaria a ser lido como liberação, sem erro. Por isso o valor 1 fica reservado, e não há renumeração.
- `importBackup` valida só `magic`, `version`, `length` e checksum. Um valor de `lastEnd` fora do intervalo não é rejeitado hoje. Não mexer nisso aqui; o mapeamento de `CER_FAREWELL` cobre o caso.

## 3.12 Botão de evolução na tela de progresso (pedido do usuário)

Arquivo: [pet_card_progress_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_progress_screen.cpp). Tamanho e estilo copiados do botão "Treinar força" ([pet_card_profile_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_profile_screen.cpp:179)).

- Botão pequeno: `fillRoundRect(133, Y, 200, 24, 9, ...)`, texto `setTextSize(2)`, centralizado em `PET_CARD_CX`. Mesmas dimensões do "Treinar força".
- Cor: `UI_BAR_BAD` usada pelo "Treinar força", ou outra cor de destaque definida na fase de implantação. Texto novo `S_EVOLVE_BTN` (nas 6 línguas).
- Visível só quando `pet.canEvolveNow()`. Se os critérios não forem atendidos, o botão não é desenhado e o toque na área não faz nada.
- Posição: abaixo do texto de evolução (`evo text` em y 256) e antes do texto de erros (y 312). Se não houver espaço, subir o texto de erros ou reduzir o espaçamento. Conferir no hardware.
- Toque: na área do botão, abrir o mesmo diálogo "evoluir ou manter" da tela principal (`choiceKind = 1`, `choiceUntil = millis() + 12000`, como em [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp:1585)). O diálogo é aberto na tela principal, então é preciso sair da tela de cartão antes (`closePetCardPages` e `setScreen`), ou reutilizar o mesmo fluxo de "Treinar força" que já fecha o cartão.
- A recusa permanente (3.9, `evoDismissed`) não esconde este botão. Ele segue a regra de `canEvolveNow()`, e o toque abre o diálogo mesmo depois de "manter forma".
- Ao tocar em "manter forma" dentro do diálogo aberto pelo progresso, o diálogo fecha e `evoDismissed` vira `true`. A tela de progresso continua mostrando o botão enquanto `canEvolveNow()` for verdadeiro.
- Não criar função nova em `Pet`: reutilizar `canEvolveNow()` e `evolveTo()`.

## 4. Fases de implantação

Cada fase deve compilar e manter o comportamento. Um commit por fase.

| Fase | Entrega | Critério de aceite |
|---|---|---|
| 0 | Compilar o estado atual e confirmar que o firmware compila sem erros. Se o `Erros.txt` ainda mostrar o erro antigo, regenerar o log | Firmware compila |
| 1 | `growthRateFromName`, `levelFromXp`, `xpCapFor` em `pet_growth.h` com testes | Testes nativos passam: limites de nível, L100 exato, XP acima do teto, cada grupo |
| 2 | `level` e `addXp` em `Pet`; `level()` retorna a variável; SFX movido | Compila; testes de `addXp` (subida, teto L100) |
| 3 | Acumulador de 15 s em `update` | 1 XP a cada 15 s acordado; zero em ovo, sono, cerimônia e offline |
| 4 | Party: `addToParty`, `replacePartySlot`, `setActiveLevel`, `switchPartySlot`, `promoteNextFromParty`, `party_screen.cpp` | Nível do slot correto ao trocar, adicionar e substituir |
| 5 | Migração `"xpv"` para ativo e party, incluindo blobs legados | Save antigo mantém o nível atual após reboot |
| 6 | Tela de progresso (3.8): barra por XP e texto `xp / próximo` | Barra zera a cada nível e cheia em L100 |
| 7 | Evolução por XP (3.9): `xpNeeded`, remoção de `evoDeclinedLv`/`evoDeclinedAge`, novo `evoDismissed` em `Pet` e `PartySlot`, persistência em NVS e backup | Evoluções ocorrem nos mesmos níveis que antes; "manter forma" some para aquela forma e volta ao evoluir |
| 8 | Outros pontos (3.10): medalhas, `TESTEVO`, `AGE`, `setActiveLevel`, party | Comandos seriais e medalhas coerentes com XP |
| 9 | Remoção da despedida (3.11): código, UI, i18n, serial e migração de `CER_FAREWELL` | Nenhuma tela ou comando de despedida; fuga e liberação funcionam |
| 10 | Botão de evolução na tela de progresso (3.12) | Botão aparece só com critérios atendidos, tem o tamanho do "Treinar força" e abre o diálogo de evolução |
| 11 | Atualizar documentação (`ARQUITETURA.md`, se existir) | Documentação reflete nível por XP, sem despedida e com botão na tela de progresso |

## 5. Testes

### 5.1 Nativos

- `levelFromXp` nos limites de cada nível, em cada grupo.
- `xpCapFor` igual a `getTotalExp(group, 100)` para cada grupo.
- `addXp` acima do teto: XP fica exatamente no teto e o nível é 100.
- `addXp` em L100: retorna 0 e não altera XP.
- Migração: `getTotalExp(group, 1 + ageMinutes/60)` produz o nível esperado.
- `xpForLevel` e a fração da barra: no início do nível a fração é 0, no fim é próxima de 1, e em L100 é 1.
- Evolução: `xp >= getTotalExp(group, minLevel + careMistakes)` dá o mesmo resultado que a regra antiga por nível.

Não testar constantes nem acessores triviais.

### 5.2 Checklist de hardware

- [ ] Pokémon acordado ganha 1 XP a cada 15 s.
- [ ] Dormindo, em ovo ou com cerimônia ativa: XP não aumenta.
- [ ] Ao subir de nível, toca o SFX e a barra reinicia.
- [ ] Em L100, XP para e o valor é exatamente o do teto do grupo.
- [ ] Reiniciar: nível e XP persistem.
- [ ] Save antigo: nível atual preservado após a migração.
- [ ] Trocar de slot na party: nível de cada membro está correto.
- [ ] Tela de progresso: barra zera a cada nível e o texto mostra `XP atual / XP do próximo`.
- [ ] Evolução: aparece no mesmo nível de antes.
- [ ] "Manter forma" fecha o diálogo e não o reabre para aquele pet, nem após reiniciar.
- [ ] Trocar para outro membro da party apto a evoluir: o diálogo aparece para ele.
- [ ] Botão de evolução na tela de progresso continua visível e abre o diálogo mesmo após "manter forma".
- [ ] Tela de progresso: botão de evolução aparece só quando os critérios são atendidos, com o tamanho do "Treinar força".
- [ ] Comando `LVL` e `LEVEL` alteram o XP e o nível mostrado.
- [ ] Save e backup com `lend`/`cerp` = 1 carregam sem erro e viram estado normal.
- [ ] Pet de forma final com 3 dias ou L100: nenhuma despedida é oferecida.
- [ ] Fuga e liberação continuam funcionando.
- [ ] Save com cerimônia de despedida pendente: pet volta ao estado normal.

## 6. Riscos

| Risco | Mitigação |
|---|---|
| Migração errada faz Pokémon cair de nível | Conversão com `getTotalExp` do nível atual e flag `"xpv"` gravada só após sucesso |
| Ritmo muito lento (174 dias para L100 no MediumFast) | Confirmar o ritmo antes de implantar. Ajustar a taxa de 15 s é uma constante |
| Nível da party desalinhado com XP | Derivar `level` sempre por `levelFromXp` ao carregar ou trocar slot |
| Espécie sem grupo na Dex | Fallback para `MediumFast` |
| Log de build desatualizado (`Erros.txt`) | Fase 0 recompila antes de qualquer mudança |
| Relógio sem RTC não avança com o aparelho desligado | Não afeta XP (offline não concede XP). Documentado em 3.6 |
| Acumulador de XP sensível a mudanças de hora | Usar `millis()` no acumulador, não `lastSeenEpoch` |
| Remover `CER_FAREWELL` do enum faria saves e backups antigos serem lidos errado | `CER_FAREWELL` fica reservado com valor 1; `lend`/`cerp` de valor 1 viram `CER_NONE` na carga e na importação (3.11) |
| Recusa "manter forma" deixa de ser adiamento e passa a ser permanente por forma (zerada ao evoluir) | Flag `evoDismissed` em `Pet` e `PartySlot`, salvo em NVS e no backup. O botão da tela de progresso (3.12) cobre a evolução |
| Layout do backup muda ao remover campos de adiamento | Verificar a validação de versão de `importBackup` antes de implantar; subir a versão se o layout mudar (3.9) |
| Remoção da despedida retira o bônus de shiny (24 vs 48) | Removido junto com a despedida, por decisão do usuário (3.11) |

## 7. Fora de escopo (futuro)

- Novas fontes de XP: batalhas, cuidados, passos, minigames.
- XP offline.
- Alterar os níveis exigidos pelas regras de evolução.
