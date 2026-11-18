# Plano de implantação: centralização do controle de hora

## 1. Objetivo

Centralizar em um único serviço (`Clock`) a leitura, escrita e cálculo de tempo do TamaPoke, hoje espalhado entre `rtcbat`, `Pet::syncClock`, `pet.lastSeenEpoch`, `app_controller.cpp`, `main_screen.cpp` e `clock_screen.cpp`.

Incluir na mesma entrega a tela de **Data e Hora**, que deve:

- sempre mostrar a data e hora atuais ao entrar;
- permitir editar dia, mês, ano, hora e minuto e aplicar a mudança no RTC.

Fora de escopo nesta fase:

- NTP e Wi-Fi (serão tratados depois, usando a interface `Clock::set`);
- alteração do layout de NVS (o save continua igual);
- fuso horário e horário de verão (o RTC é tratado como UTC).

## 2. Situação atual (resumo)

| Ponto | Situação |
|---|---|
| Hora persistente | RTC PCF85063 via `rtcEpoch()` / `rtcSetEpoch()` em `rtcbat.cpp` |
| Baseline offline | `pet.lastSeenEpoch`, salvo na NVS com a chave `"seen"` (`pet.cpp`, `Pet::save()`) |
| Catch-up offline | `Pet::syncClock` aplica decaimento minuto a minuto, limitado a 14 dias |
| Escrita direta | `app_controller.cpp` (linhas 192–193) grava `pet.lastSeenEpoch` diretamente |
| Fallback | `main_screen.cpp` (~linha 252) usa `rtcEpoch()` ou, se falhar, `pet.lastSeenEpoch` |
| Conversão de data | `settings_logic.h` (`settingsDateTimeFromEpoch` / `settingsDateTimeEpoch`) e `dayphase.h` duplicam aritmética de calendário |
| Timers de UI | ~80 usos de `millis()` com `deadlineActive(...)` |
| Tela de data/hora | `clock_screen.cpp`: `open()` lê o RTC; `applyTime()` chama `rtcSetEpoch` e depois `pet.setClock` |

Problemas observados:

1. Duas fontes de verdade para a última hora vista (`rtcEpoch` e `lastSeenEpoch`).
2. Escrita de baseline fora do `Pet`.
3. Conversão epoch↔calendário em três lugares.
4. A tela só lê o RTC no `open()`. Se o RTC estiver inválido, a tela mostra "RTC ERROR" e não deixa editar.

## 3. Arquitetura proposta

### 3.1 Novos arquivos

| Arquivo | Responsabilidade |
|---|---|
| `src/core/clock.h` | Lógica pura, sem Arduino nem hardware: `ClockDateTime`, `clockDateTime(epoch)`, `epochFromDateTime(dt)`, `elapsedMinutes(last, now, capMinutes)` |
| `src/core/clock.cpp` | Serviço `Clock` (namespace): `Clock::nowEpoch()`, `Clock::dateTime()`, `Clock::set(epoch)`, `Clock::isValid()`. Obtém e grava a hora somente através de `src/hardware/rtc.h` |
| `src/core/deadline.h` | Wrapper `Deadline` sobre `millis()` (substitui `deadlineActive(millis(), x)` nos call sites) |
| `src/core/tick_scheduler.h` | `TickScheduler`: encapsula o acumulador de `PET_TICK_MS` (60 s) usado em `Pet::update` |
| `src/hardware/rtc.h` / `rtc.cpp` | Única camada que acessa o PCF85063. Expõe `rtcReadEpoch()`, `rtcWriteEpoch(epoch)` e `rtcIsPresent()`. Encapsula o que hoje está em `rtcbat.cpp` (`rtcEpoch`, `rtcSetEpoch`, validação de ano e cache de energia) |

Regra de dependência: `src/core` não inclui nada de `src/hardware` diretamente, exceto `clock.cpp`, que é o único ponto de ligação. Telas (`src/ui`) e `Pet` usam `Clock::`, nunca `rtc*` diretamente. Se algum outro código depender de configuração de hardware (pinos, endereço I2C, bateria do RTC), ele também deve ficar em `src/hardware`.

A migração move `rtcbat.cpp` / `rtcbat.h` para `src/hardware/`, mantendo o comportamento. A tela de Data e Hora continua em `src/ui/clock_screen.cpp`.

Regra de `ARQUITETURA.md`: lógica pura fica em `.h` e é testada em `tests/`.

### 3.2 Contratos

```cpp
struct ClockDateTime { int year, month, day, hour, minute, second; };

ClockDateTime clockDateTime(uint32_t epoch);
uint32_t      epochFromDateTime(const ClockDateTime &dt);
uint32_t      elapsedMinutes(uint32_t lastEpoch, uint32_t nowEpoch, uint32_t capMinutes);

namespace Clock {
  bool          isValid();              // RTC presente e ano dentro de 2025–2120
  uint32_t      nowEpoch();             // 0 se inválido
  ClockDateTime dateTime();             // atual, sem cache
  bool          set(uint32_t epoch);    // grava no RTC e retorna sucesso
}
```

- `elapsedMinutes` retorna 0 quando `nowEpoch <= lastEpoch` (relógio andou para trás ou não mudou).
- `elapsedMinutes` aplica o teto `capMinutes` (14 dias = `14UL * 24 * 60`).

### 3.3 Mudanças em `Pet`

- Criar `Pet::applyOfflineMinutes(uint32_t minutes)` com a lógica de decaimento hoje dentro de `syncClock`.
- `Pet::syncClock(epoch)` vira um wrapper: calcula `elapsedMinutes(lastSeenEpoch, epoch, cap)`, chama `applyOfflineMinutes` e atualiza `lastSeenEpoch`.
- Remover a escrita direta de `lastSeenEpoch` em `app_controller.cpp`. Quem grava baseline passa a ser só `Pet`.
- Não alterar o formato salvo: `lastSeenEpoch` continua sendo serializado no mesmo lugar e com a mesma chave `"seen"`.

### 3.4 Mudanças na tela de Data e Hora (`clock_screen.cpp`)

Comportamento alvo:

1. **Ao entrar** (`open()`): ler `Clock::dateTime()` e preencher os campos. Não usar cópia antiga.
2. **Enquanto aberta**: re-ler o RTC a cada ~1 s para mostrar o relógio andando, **somente enquanto o usuário não tiver tocado em nenhum campo**. Depois de editar, congelar os campos até OK ou sair. Isso evita que a tela sobrescreva o que o usuário está digitando.
3. **RTC inválido**: mostrar "RTC ERROR" na linha de status, mas manter os campos editáveis com a data de referência (`pet.lastSeenEpoch`), para permitir ajustar a hora. Aplicar com `Clock::set`.
4. **OK**: converter com `epochFromDateTime`, chamar `Clock::set(epoch)`. Só se `set` retornar sucesso, chamar `pet.syncClock(epoch)` para não gerar catch-up falso. Fechar a tela.
5. **Swipe vertical / sair**: fechar sem aplicar.
6. Validação: `settingsAdjustDateTime` já limita o dia ao máximo do mês. Após o ajuste de ano, confirmar que o dia não ultrapassa o mês (já feito em `settingsAdjustDateTime`).

Decisões confirmadas pelo usuário:

- Ao aplicar uma hora **anterior** à hora atual, `lastSeenEpoch` recebe o novo epoch. `elapsedMinutes` retorna 0, então não há progressão extra.
- Ao aplicar uma hora **posterior**, o catch-up continua limitado a 14 dias (`14UL * 24 * 60`), e isso deve ser documentado.
- A faixa de ano da tela é 2025–2099, alinhada com a validade do RTC. Como `rtcEpoch` aceita até 2120, a tela limita em 2099 e a validação do RTC permanece como está.

### 3.5 Substituições de tempo (sem mudança de comportamento)

- `dayphase.h` (raiz): `unixDayFromEpoch` e `unixHourFromEpoch` passam a usar `clockDateTime` de `src/core/clock.h`.
- `src/ui/settings_logic.h`: `settingsDateTimeFromEpoch` e `settingsDateTimeEpoch` viram wrappers finos de `clockDateTime` / `epochFromDateTime`, ou são removidos se a tela migrar para `ClockDateTime`.
- `time_utils.h`: manter só os helpers de deadline. Ajustar a descrição em `ARQUITETURA.md`, que hoje diz que ele contém funções de hora do dia.
- `main_screen.cpp`: trocar o fallback `rtcEpoch() ?: pet.lastSeenEpoch` por `Clock::nowEpoch()` com tratamento explícito de inválido.

## 4. Fases de implantação

Cada fase deve compilar e manter o comportamento. Um commit por fase.

| Fase | Entrega | Critério de aceite |
|---|---|---|
| 1 | `clock.h` com funções puras e `tests/clock_test.cpp` | Testes nativos passam: anos bissextos (1900 não, 2000 sim, 2024 sim), fim de mês, virada de ano, meia-noite, `elapsedMinutes` com cap e com relógio para trás |
| 2 | `src/hardware/rtc.h/.cpp` (move de `rtcbat`) e `src/core/clock.cpp` (serviço `Clock`), sem uso ainda | Compila no ESP32; `rtcbat` removido ou reexportado sem mudança de comportamento |
| 3 | `Pet::applyOfflineMinutes` e `syncClock` como wrapper | Comportamento offline idêntico (testes `pet_growth_test.cpp` e `pet_stats_test.cpp` continuam passando) |
| 4 | Remover escrita direta de `lastSeenEpoch` em `app_controller.cpp` e fallback em `main_screen.cpp` | Sem regressão no boot e na tela principal |
| 5 | Tela de Data e Hora (seção 3.4) | Ver checklist de hardware |
| 6 | `Deadline` e `TickScheduler` | Sem mudança de timing; `PET_TICK_MS` mantido |
| 7 | Consolidar `dayphase.h` (raiz) e `src/ui/settings_logic.h`, atualizar `ARQUITETURA.md` | Nenhum símbolo antigo sobrando; `grep` por `settingsDateTimeFromEpoch` e `unixDayFromEpoch` sem uso fora de wrappers |

Fases 6 e 7 são opcionais se o prazo apertar. A fase 5 é a entrega principal pedida.

## 5. Testes

### 5.1 Nativos (`tests/clock_test.cpp`)

Testar apenas comportamento novo:

- `clockDateTime(epochFromDateTime(dt)) == dt` para datas nos limites: 2024-02-29, 2025-12-31 23:59:59, 2026-01-01 00:00:00.
- `elapsedMinutes`: retorna 0 quando `now <= last`; retorna o cap quando a diferença excede o cap.
- Não testar constantes nem acessores triviais.

### 5.2 Checklist de hardware

- [ ] Entrar na tela: a data e a hora mostradas batem com o RTC.
- [ ] Deixar a tela aberta por 1 min sem tocar: o relógio avança.
- [ ] Tocar em um campo: os campos param de atualizar.
- [ ] Ajustar, aplicar com OK: o valor aparece na tela principal.
- [ ] Reiniciar: a hora persiste.
- [ ] Com RTC inválido (simular ano fora da faixa): aparece "RTC ERROR", campos editáveis, OK grava.
- [ ] Sair com swipe: nada é alterado.
- [ ] Aplicar hora posterior: o catch-up respeita o cap de 14 dias.

## 6. Documentação

- Atualizar `ARQUITETURA.md`:
  - adicionar `Clock`, `Deadline` e `TickScheduler` na lista de módulos;
  - corrigir a descrição de `time_utils.h`;
  - registrar que a escrita de `lastSeenEpoch` é exclusiva de `Pet`.
- Não editar arquivos gerados (`dex.h`, `species.h`) à mão.
- Não incluir comandos de teste (`TESTMON`, `TESTEVO`) em builds públicos. A ferramenta de `RTCSET` em `serial_console.cpp` deve continuar restrita à build de teste.
- Atualizar `FW_VERSION` junto com os manifests quando a versão for publicada.

## 7. Riscos

| Risco | Mitigação |
|---|---|
| Tela sobrescrever edição do usuário | Congelar atualização após o primeiro toque |
| Catch-up falso ao trocar a hora | Chamar `syncClock` só após `Clock::set` retornar sucesso |
| Regressão no decaimento offline | Testes nativos existentes e fase 3 isolada |
| Ano fora de 2025–2099 na tela | Alinhar faixa de ano com a validade do RTC |
| RTC sem bateria | Tela continua funcional com a data de referência e aviso visível |

## 8. Fora de escopo (futuro)

- NTP / SNTP com Wi-Fi (`configTime`, validação, `Clock::set`). O design acima já prevê a interface necessária.
- Armazenamento de credenciais Wi-Fi na NVS.
- Fuso horário e horário de verão.

## 9. Desvios da implementação

- Arquivo do serviço: `src/core/clock_service.h/.cpp` (em vez de `clock.cpp`), porque `clock.h` precisa ficar puro para os testes nativos.
- `Pet::applyOfflineMinutes(uint32_t mins, uint32_t startEpoch)`: precisa do epoch inicial para a fonte da comida noturna.
- `Pet::observeClock(nowEpoch)` foi adicionado: atualiza `lastSeenEpoch` sem salvar e sem progressão.
- `ClockDateTime` tem campo `second`. `applyTime` zera os segundos, mantendo o comportamento antigo.
- Tela de data/hora: `syncClock` agora aplica o catch-up, limitado a 14 dias. Antes usava `setClock`, sem catch-up.
- Tela de data/hora: atualiza uma vez por segundo enquanto não há toque. Pode piscar no dispositivo (não verificado).
- `TickScheduler` (`src/core/tick_scheduler.h`) substitui o `lastTick` de `Pet`.
- `Deadline` não foi implementado: são cerca de 80 pontos de uso de `deadlineActive(millis(), ...)`, com alto custo de troca e sem ganho de comportamento. Adiado.
- `dayphase.h` não foi alterado: usa divisões simples, não cálculo de calendário.
- `pet.setClock` continua em uso pelos comandos seriais `TIME` e `RTCSET`, que são ferramentas de desenvolvimento.
- Compilação do firmware Arduino não verificada (sem toolchain no ambiente).
- Boot com RTC ausente, parado ou fora de 2025–2120 (`Clock::nowEpoch() == 0`): abre `ClockSettingsScreen` automaticamente (`openAtBoot`, sem som). O antigo seed silencioso de 2026-01-01 foi removido. Nesse caso `syncClock` não roda, para não gravar `lastSeenEpoch = 0`. Ao confirmar a hora, `applyTime` chama `syncClock` e aplica o catch-up normal. Se havia tela de iniciante pendente, ela volta ao fechar. Com RTC presente mas zerado, a tela mostra "hora não configurada" (traduzida por idioma, em `UNSET_LABELS`). Com RTC ausente, mostra "RTC ERROR". Em ambos os casos a tela continua editável.