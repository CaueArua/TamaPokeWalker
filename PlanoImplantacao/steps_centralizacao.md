# Plano de implantação: centralização dos passos diários

## 1. Objetivo

Tirar o controle de passos diários de dentro de `Pet` e colocá-lo em uma classe própria, `DailyActivity`, que representa as atividades diárias do dispositivo. Nesta fase, a classe guarda apenas os **passos dados no dia**, junto com o dia de registro.

Requisitos definidos pelo usuário:

1. Os passos são separados do `Pet`. A nova classe representa as atividades diárias. Por ora, contém somente os "passos dados no dia".
2. O valor deve ser persistido, para que o dispositivo retome de onde parou após desligar.
3. A classe tem um campo com o dia de registro (análogo ao `lastSeenEpoch`). Quando o dia atual for maior que o dia registrado, os contadores internos são zerados.
4. A classe é atualizada a cada loop e também quando data/hora forem alteradas.

## 2. Situação atual

- Os passos diários ficam em `Pet` (`stepsToday`, `stepDay`, `stepDailyRewardMask`) em [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h), por volta das linhas 239-243.
- O rollover do dia é feito por `Pet::ensureStepDay()` em [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp), por volta da linha 1313. Ele usa `today()` (`lastSeenEpoch / 86400`), e não o relógio RTC.
- `Pet::applyWalk(steps, nowEpoch)` em [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp), por volta da linha 1418, soma os passos diários e totais, trata recompensas, marcos da trilha, expedições e JOY/BOND.
- `applyWalk` é usado de duas formas:
  - Ativamente no loop principal, em [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp) linha 137 (`pet.applyWalk(walked, Clock::nowEpoch())`), sempre que o IMU reporta passos.
  - Pelo comando serial de desenvolvimento `WALK <n>`, em [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp) linha 457.
- `Pet::observeClock` é chamado a cada 30 s (60 s em power save), em [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp) linhas 191-195.
- O relógio é a API `Clock` em [clock_service.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/core/clock_service.h): `begin`, `available`, `nowEpoch`, `set`, `tick`. `Clock::nowEpoch()` retorna 0 quando a hora é desconhecida.
- Persistência atual em `Pet::save`/`load` (NVS): chaves `stday`, `stoday`, `stotal`, `stdone`, `stmil`, `stpend`, `exsteps`.

## 3. Arquitetura proposta

### 3.1 Nova classe `DailyActivity`

Arquivo: `src/core/daily_activity.h` (lógica pura, sem hardware), seguindo o padrão do `clock_service.h`.

Campos:

- `dayIndex` (uint32): dia de registro, calculado como `epoch / 86400`. Equivale ao `lastSeenEpoch` usado hoje.
- `stepsToday` (uint32): passos dados no dia de `dayIndex`.

Métodos:

- `update(nowEpoch)`: se `nowEpoch == 0`, não faz nada. Se `nowEpoch / 86400 > dayIndex`, zera `stepsToday`, atualiza `dayIndex` e sinaliza a virada de dia. Retorna `true` quando houve virada.
- `addSteps(steps, nowEpoch)`: chama `update(nowEpoch)` e depois soma com saturação.
- `stepsToday()`: somente leitura.
- `dayChangedFlag()` (ou retorno de `update`): permite que `Pet` reset a máscara de recompensas diárias (`stepDailyRewardMask`) no momento da virada, sem depender de `ensureStepDay`.

### 3.2 Integração

- `Pet` deixa de guardar `stepsToday` e `stepDay`. Passa a ler `DailyActivity::stepsToday()`.
- `Pet::applyWalk` recebe os passos já contados pela `DailyActivity` (ou a `DailyActivity` é chamada antes, no mesmo ponto). A regra de recompensas diárias continua em `Pet`, mas a virada de dia é notificada pela `DailyActivity`.
- `Pet` mantém: `stepsTotal`, marcos da trilha, máscaras de recompensa, expedições, JOY/BOND. Estes não fazem parte desta fase.
- O objeto `DailyActivity` vive no controlador do app (`app_controller`) e é passado ao `Pet`, ou fica como membro de um serviço leve. A decisão está na seção 8.

### 3.3 Atualização a cada loop e em mudança de hora

- Em [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp), no loop principal, chamar `dailyActivity.update(Clock::nowEpoch())` antes de processar os passos do IMU.
- Ao alterar data/hora, chamar `dailyActivity.update(...)` imediatamente. Pontos de alteração:
  - Comandos serial `TIME` e `RTCSET` em [serial_console.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/serial_console.cpp), linhas 110-121.
  - Tela de ajuste de relógio em `src/ui/clock_screen.cpp` (`applyTime` → `Clock::set` → `pet.syncClock`).
  - Hoje já existe `Pet::setClock` / `Pet::syncClock`. A `DailyActivity` deve ser atualizada no mesmo ponto.

### 3.4 Persistência

- Manter as chaves NVS existentes `stday` (dia de registro) e `stoday` (passos do dia). Assim não há migração: o valor salvo hoje é lido pela nova classe.
- Salvar a `DailyActivity` em três situações:
  - Quando o dia vira.
  - Quando os passos cruzam um múltiplo de 100 (comportamento atual, `pendingSave`).
  - Ao alterar data/hora.
- Risco de perda: no máximo 99 passos podem se perder se o dispositivo for desligado entre dois salvamentos. Ver seção 7.

### 3.5 Regras de virada de dia

- Dia atual > dia registrado: zera `stepsToday`, zera a máscara de recompensas diárias e registra o novo dia.
- Dia atual == dia registrado: não faz nada.
- Dia atual < dia registrado (relógio voltou): não zera. Proposta: manter os contadores e não mexer no dia registrado. Ver seção 8.
- Hora desconhecida (`nowEpoch == 0`): não rola o dia. Passos continuam somando no dia registrado.

## 4. Fases

### Fase 1: Classe pura e testes
- Criar `src/core/daily_activity.h` com `update`, `addSteps`, `stepsToday` e a sinalização de virada.
- Testes nativos em `tests/` cobrindo: mesma dia sem virada, virada com zeragem, virada sem passos, `nowEpoch == 0`, relógio voltando, saturação de `uint32`, e ida e volta de save/load (campos `dayIndex` e `stepsToday`).

### Fase 2: Integração com `Pet`
- Remover `stepsToday` e `stepDay` de `Pet`. Ler a partir da `DailyActivity`.
- Substituir `ensureStepDay` pela sinalização da virada.
- Ajustar `applyWalk`, `stepShinyChancePer4096`, `stepCatchBonus`, `updateWalkCards` e as telas que leem `stepsToday`:
  - [main_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/main_screen.cpp)
  - [pet_card_steps_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_steps_screen.cpp)
  - [battle_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/battle_screen.cpp)
  - [pet_card_expedition_screen.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/ui/pet_card_expedition_screen.cpp)

### Fase 3: Loop e mudança de hora
- Chamar `update` a cada loop em `app_controller.cpp`.
- Chamar `update` em `TIME`, `RTCSET` e `clock_screen.cpp`.
- Garantir que `observeClock` (30 s / 60 s) continue existindo para JOY/BOND e `lastSeenEpoch`, sem conflito com a nova fonte do dia.

### Fase 4: Persistência
- Salvar e carregar `stday` e `stoday` pela nova classe, mantendo as mesmas chaves NVS.
- Validar o salvamento na virada de dia e na mudança de hora.

### Fase 5: Documentação
- Atualizar [ARQUITETURA.md](C:/Users/cauea/Documents/projetos/TamaPokeExtended/ARQUITETURA.md), linha 69 (fluxo de passos: hoje diz "Passos contados vão para `pet.applyWalk()`").
- Documentar a `DailyActivity` no mesmo lugar onde o relógio está descrito.

## 5. Testes

Testes nativos (sem hardware) cobrindo apenas comportamento novo:

- Virada de dia zera `stepsToday` e sinaliza a mudança.
- Mesmo dia não zera.
- `nowEpoch == 0` não rola o dia.
- Relógio voltando não zera (conforme decisão da seção 8).
- Passos são somados ao dia atual com saturação.

Checklist no hardware:

- Andar, desligar, ligar: os passos do dia continuam.
- Ajustar a hora para o dia seguinte com `TIME`: os passos zeram e a máscara de recompensas diárias também.
- Ajustar a hora pela tela: mesmo comportamento.
- Passos sem hora conhecida: continuam somando.

## 6. Fora de escopo

- Passos totais, marcos da trilha, expedições e JOY/BOND permanecem em `Pet` nesta fase.
- Mudança no formato do save além de `stday`/`stoday`.
- Alterações de UI além da troca de leitura.

## 7. Riscos

- **Leitura do RTC a cada loop**: custo de I2C. Medir antes; se for alto, guardar o último valor e só consultar o RTC a cada N ms, mantendo a atualização imediata na mudança de hora.
- **Perda de até 99 passos** ao desligar: o salvamento atual é a cada 100 passos. Opções: salvar a cada passo (desgaste da NVS) ou a cada N passos com flush periódico.
- **Duas fontes de dia**: `Pet` usa `lastSeenEpoch` para JOY/BOND e a nova classe usa `Clock::nowEpoch()`. Podem divergir por até 30 s. Recomendo que JOY/BOND continuem com a fonte atual nesta fase.
- **Máscara de recompensas**: precisa resetar na mesma virada. Se a notificação falhar, a recompensa do dia anterior não volta a ser concedida, o que é o comportamento desejado.

## 8. Decisões pendentes (confirmar com o usuário)

1. **Dono da instância da `DailyActivity`**: diretamente no `app_controller` (recomendado) ou em um serviço global como `Clock`?
2. **Relógio que volta no tempo**: manter os contadores e não alterar o dia registrado (recomendado), ou zerar?
3. **Frequência de salvamento**: manter o salvamento a cada 100 passos (menos desgaste, risco de perder até 99 passos) ou salvar a cada passo?
4. **Máscara de recompensas diárias**: fica em `Pet` e é resetada pela notificação da virada (recomendado), ou passa para a `DailyActivity`?
5. **Escopo da fase**: confirmar que passos totais e marcos continuam em `Pet`.
