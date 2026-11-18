# TamaPoke – Arquitetura do projeto

Documento de referência para desenvolvimento. Descreve a estrutura do firmware,
a função de cada arquivo e as funcionalidades implementadas.

> Escopo: firmware Arduino/C++ para o **Waveshare ESP32-S3-Touch-AMOLED-1.75**
> (tela AMOLED redonda 466×466, touch CST9217, PMU AXP2101, RTC PCF85063, IMU QMI8658,
> codec ES8311 e microSD).

## 1. Visão geral

TamaPoke é um pet virtual inspirado em Pokémon. O jogador cuida do pet (alimentação,
alegria, energia e higiene), treina, participa de batalhas e minijogos, captura
Pokémon selvagens, evolui o pet, faz expedições e ganha recompensas andando com o
dispositivo.

O firmware é organizado em camadas:

```
TamaPokeExtended.ino          Ponto de entrada (setup / loop)
        │
src/app_controller.cpp        Loop principal: atualiza pet, sensores, render, energia
src/startup.h, hardware_setup, program_setup    Inicialização em ordem de dependência
        │
src/ui/*                      Telas (Screen), navegação, toque, renderização, energia
        │
pet.h / pet.cpp               Regras de jogo, progressão, persistência (NVS)
battle.h / battle.cpp         Matemática de batalha (determinística)
dex.h, species.h              Dados de espécies, evoluções, sprites (gerados)
        │
Drivers de hardware           audio, imu, sdmon, thumb_format, pin_config (RTC e AXP2101 em src/hardware/)
i18n.h / i18n.cpp             Textos em 6 idiomas
serial_console.cpp            Comandos de depuração e transferência via USB serial
```

Dependências importantes:

- `TamaPokeExtended.ino` inclui `Arduino_GFX_Library.h`, cria o barramento QSPI, o
  painel CO5300 e o canvas em PSRAM (`gfx`), e instancia as globais `pet`, `mon`,
  `pmd`, `evoPmd` e `beh`.
- Quase todo estado compartilhado entre telas é declarado como `extern` em
  `.h` e definido no `.cpp` ou no `.ino`.
- Dados de espécies (`dex.h`) e sprites (`species.h`) **não devem ser editados
  à mão** (ver seção 4).

## 2. Fluxo de execução

### 2.1 `setup()` (em `TamaPokeExtended.ino`)

Chama, nesta ordem:

1. `initializeHardware()` (`src/hardware_setup.cpp`): Serial 115200 com TX sem
   bloqueio, idioma salvo, I2C (`Wire`), habilita o rail BLDO1 do painel
   (`pmuEnablePanel`) e inicia o display em QSPI de 80 MHz.
2. `initializeSensors()` (declarado em `src/startup.h`): touch, RTC, bateria e IMU.
3. `initializeProgram()` (`src/program_setup.cpp`): carrega o pet (`pet.begin()`),
   abre a tela de escolha de starter se necessário, monta o SD (`sdBegin`), carrega
   miniaturas, configura energia, sincroniza o relógio com o tempo offline
   (`pet.syncClock`), renderiza e inicia o áudio (`audioBegin`).

### 2.2 `loop()` → `runAppLoop()` (`src/app_controller.cpp`)

A cada iteração:

1. `pet.update(now)` avança a simulação do pet (1 tick = 1 minuto de jogo).
2. Detecta eventos sonoros (pet apto a evoluir, risco de fuga).
3. `pollTouch()`, `handleSerial()`, `ensureMon()` (recarrega sprite se a espécie mudou).
4. `mainScreen.update()`, sons ambientes, IMU (`imuPoll`, `imuTakeSteps`, `imuShakeEdge`).
5. Passos contados vão para `pet.applyWalk()`.
6. PWR curto alterna a tela ligada/desligada.
7. `updateBrightness`, salvamento periódico de NVS (só com tela apagada/escurecida/pet dormindo),
   relógio do RTC, `mainScreen.updateExpedition`.
8. Heartbeat `HEALTH ...` na serial a cada 5 min (10 min em modo economia).
9. `render()` no intervalo de `renderIntervalMs()`.
10. `maybeLightSleep()` quando permitido.

## 3. Arquivos da raiz

| Arquivo | Função |
| --- | --- |
| `TamaPokeExtended.ino` | Ponto de entrada Arduino. Define `FW_VERSION` (`1.35.3-soft-step` por padrão; `1.36.2` no perfil de release Gen 3; `1.36.2-hoenn-walk-local` no perfil de teste local), cria o painel/canvas e as globais de sprite, e chama `setup`/`loop`. |
| `pin_config.h` | Mapeamento de pinos (QSPI da tela, I2C, I2S, SD, PMU) da placa Waveshare. |
| `pet.h` / `pet.cpp` | **Núcleo do jogo**: classe `Pet` (estado, simulação, persistência em NVS via `Preferences`, grupo, evolução, captura, expedição, passos, medalhas, metas diárias). `pet.cpp` é o maior arquivo do projeto (~75 KB). |
| `battle.h` / `battle.cpp` | Regras de batalha por turnos: cálculo de stats, nível do selvagem, tabela de encontros por fase do dia, efetividade de tipos, `stepBattle` (um turno) e `resolveBattle` (batalha completa e determinística, com `luckRoll`). |
| `dex.h` | **Gerado** por `tools/gen_dex.py` a partir de `tools/dex_data.py`. Contém a Pokédex (386 espécies de Kanto, Johto e Hoenn), tipos, raridade, bioma, regras de evolução e nomes localizados (ES, EN, FR, DE, IT, PT). |
| `species.h` | **Gerado** por `tools/sprites.py emit`. Tabela de espécies e dados de sprite monocromático. |
| `species_chirp.h` / `species_chirp.cpp` | Chirps sintéticos por espécie (áudio gerado por código, sem samples originais). |
| `audio.h` / `audio.cpp` | Driver de áudio: codec ES8311, I2S, amplificador, tarefa FreeRTOS que toca uma fila de efeitos sonoros (`Sfx`) sem bloquear o loop. Modos de volume em `SoundMode`. |
| `i18n.h` / `i18n.cpp` | Sistema de textos: enum `StrId` (ordem deve coincidir com a tabela `STRINGS`), `T(id)`, `dexName`, nomes/descrições de medalhas e idioma persistido (`loadLang`, `setLang`). |
| `imu.h` / `imu.cpp` | Driver do QMI8658: detecção de passos (com filtro de cadência para rejeitar trepidações isoladas), detecção de sacudida (*shake*) e contadores de pedômetro. |
| `time_utils.h` | Helpers de deadline (`deadlineActive`, etc.) usados pelos timers de UI e simulação. |
| `dayphase.h` | Fases do dia (manhã, dia, tarde, noite) e helpers de visual noturno e perda de comida. |
| `sdmon.h` / `sdmon.cpp` | Gerenciamento do microSD: montagem (SDMMC 1 bit), carregamento de sprites `TPK1` (legado, `SdMon`) e `TPK2`/PMD (`PmdMon`, multi-ação, na PSRAM), índice de miniaturas (`SdThumbs`) e comandos de transferência via serial. |
| `thumb_format.h` / `thumb_format.cpp` | Formato codificado das miniaturas da galeria. |
| `partitions.csv` | Tabela de partições da flash (app de 3 MB + FAT de ~9 MB + NVS). |
| `.gitignore` | Ignora artefatos de build. |
| `README.md` | Documentação pública: instalação pelo instalador web, controles, hardware, créditos. |
| `progress.md` | Acompanhamento de progresso e caminhos do instalador. |
| `CREDITS.md` | Créditos de sprites (PMD SpriteCollab, CC BY-NC 4.0), modelo 3D e projeto original. |
| `LICENSE` | Licença MIT do código-fonte. |
| `Erros.txt` | Registro de um erro de compilação antigo (`pet` não declarado em `src/ui/scene_renderer.cpp`). Não é código; útil apenas como histórico. |

## 4. Arquivos gerados e cuidados

- **`dex.h`** e **`species.h`** são gerados. Para alterar espécies, evoluções ou sprites,
  altere a fonte e regenere. Os scripts citados (`tools/gen_dex.py`, `tools/dex_data.py`,
  `tools/sprites.py`) são referenciados no código e no README, mas **a pasta `tools/` não
  existe neste checkout**.
- **Persistência (NVS)**: alterações em `PartySlot`, `Pet` ou nas chaves de `Preferences`
  podem corromper saves. Antes de mudar o layout, planeje migração.
- Os documentos `AGENTS.md`, `AI_HANDOFF.md`, `CLAUDE.md`, `HOENN_WALK_LOCAL.md`,
  `.nojekyll` e `esquentadoBkp.txt` aparecem como removidos na árvore de trabalho do git
  (`git status`), mas continuam no histórico.

## 5. Pasta `src/`

### 5.1 Inicialização e controle

| Arquivo | Função |
| --- | --- |
| `src/startup.h` | Declara `initializeHardware`, `initializeSensors` e `initializeProgram`. |
| `src/hardware_setup.cpp` | Implementa `initializeHardware`: serial, idioma, I2C, painel e brilho inicial. |
| `src/program_setup.cpp` | Implementa `initializeProgram`: pet, SD, miniaturas, energia, relógio, render e áudio. (`initializeSensors` está declarada aqui, mas não há implementação neste arquivo.) |
| `src/sensor_setup.cpp` | Inicialização dos sensores (touch, RTC, bateria, IMU). |
| `src/app_controller.h` / `.cpp` | Loop principal (`runAppLoop`) e lógica de som ambiente. Mantém estado de sprite (`ensureMon`). |
| `src/serial_console.h` / `.cpp` | `handleSerial()`: lê linhas da USB serial e despacha comandos. |
| `src/core/clock.h` | Lógica pura de calendário e tempo offline (sem Arduino). Testada em `tests/clock_test.cpp`. |
| `src/core/clock_service.h` / `.cpp` | Namespace `Clock`: `nowEpoch()`, `set()` e `tick()`. Usa o RTC quando válido; sem RTC, conta segundos com `millis()` a partir da última hora salva. Único arquivo de `core` que fala com o hardware. |
| `src/core/tick_scheduler.h` | Contador de ticks de período fixo, usado por `Pet::update`. Testado em `tests/tick_scheduler_test.cpp`. |
| `src/hardware/rtc.h` / `.cpp` | Driver do RTC PCF85063 (`rtcReadDateTime`, `rtcWriteDateTime`). |
| `src/hardware/soft_clock_store.h` / `.cpp` | Grava e lê a hora de software no NVS (namespace `tamapoke`, chave `clkSoft`). |
| `src/hardware/power.h` / `.cpp` | Driver do AXP2101: bateria, USB, painel e botão PWR. |

Regra: telas e `Pet` usam `Clock::` ou `hardware/power.h`. Não acessam `rtc*` diretamente.

Comandos seriais identificados em `serial_console.cpp`: `SAVEPUT`, `SPEC`, `LVL`, `TIME`,
`RTCSET` (simula tempo desligado), `NICK`, `CAUGHT`, `BATTLE`, `WALK` e os de teste
`TESTMON` / `TESTEVO` (**somente em builds de teste; não podem existir no instalador público**).
Comandos de SD são tratados por `sdSerialCommand` (`sdmon.h`).

## 6. Pasta `src/ui/`

### 6.1 Base de telas e navegação

| Arquivo | Função |
| --- | --- |
| `screen.h` / `screen.cpp` | Classe base `Screen` (ciclo `onEnter`/`render`/`onExit`, toques, pressão, long press, swipes horizontal/vertical, `markDirty`, `isStaticClean`). Define `activeScreen` e `setScreen()`. |
| `screen_controller.h` / `.cpp` | Roteamento de telas: `render()`, `onTap`, `onSwipe`, `onSwipeV`, `renderIntervalMs()` e `screenName()` (usado nos heartbeats). Decide quando pular render de telas estáticas. |
| `touch_controller.h` / `.cpp` | `pollTouch()`: lê o CST9217 (com gate pelo INT), detecta toque, long press e swipes, e entrega ao `screen_controller`. |
| `scene_renderer.h` / `.cpp` | Desenho da cena do jogo (bioma, hora do dia, fundo). |
| `render_utils.h` / `.cpp` | Helpers de desenho compartilhados (barras, textos, botões, formatação). |
| `power_manager.h` / `.cpp` | Brilho em estágios (`dimStage`), tela apagada (`screenOff`), economia de energia (`powerSave`), *light sleep* e atualização do brilho. |
| `settings_logic.h` | Funções puras (testáveis) para volume, brilho, data e hora. |

### 6.2 Tela principal e cartões do pet

| Arquivo | Função |
| --- | --- |
| `main_screen.h` / `.cpp` | **Tela inicial** (maior arquivo de UI, ~57 KB). Desenha o pet, botões, eventos aleatórios, encontro selvagem, evolução, fuga, balanço de passos, HUD de expedição, banho e liberação. Classe `MainScreen` e globais de comportamento (`beh`). |
| `pet_card_page.h` / `.cpp` | Container de cartões deslizáveis (página atual `cardPage`). |
| `pet_card_common.h` / `.cpp` | Helpers comuns aos cartões. |
| `pet_card_summary_screen.cpp` | Cartão de resumo (novo, ainda não rastreado pelo git). |
| `pet_card_profile_screen.cpp` | Perfil: nome, idade, informações e renomear. |
| `pet_card_personality_screen.cpp` | Personalidade (balanced, playful, brave, calm, lazy). |
| `pet_card_daily_screen.cpp` | Metas diárias (cuidado, brincadeira, batalha, captura, memo). |
| `pet_card_box_screen.cpp` | Caixa de capturados, filtros e ações. |
| `pet_card_stats_screen.cpp` | Estatísticas de batalha (ATK/DEF/SPE/peso) e treinamento. |
| `pet_card_progress_screen.cpp` | Progresso de nível e requisitos de evolução. |
| `pet_card_steps_screen.cpp` | Passos de hoje/total, recompensas e rank de trilha. |
| `pet_card_medals_screen.cpp` | Medalhas conquistadas. |
| `pet_card_expedition_screen.h` / `.cpp` | Expedições (costa, vulcão, floresta, sítios de Hoenn), inventário e recompensas de passos. |

### 6.3 Menus, atividades e configurações

| Arquivo | Função |
| --- | --- |
| `menu_screen.h` / `.cpp` | Menu principal (grade de opções). |
| `activity_menu_screen.h` / `.cpp` | Menu de atividades (alimentar, brincar, banho, luz, etc.). |
| `settings_screen.h` / `.cpp` | Configurações: som, brilho (prévia), economia de energia, data e hora, versão. |
| `clock_screen.h` / `.cpp` | Ajuste de data e hora (campos dia/mês/ano/hora/minuto). |
| `help_screen.h` / `.cpp` | Tela de ajuda com controles. |
| `keyboard_screen.h` / `.cpp` | Teclado virtual para dar nome ao pet. |
| `party_screen.h` / `.cpp` | Equipe (até 6 Pokémon); inclui `partyReplaceScreen`. |
| `starter_screen.h` / `.cpp` | Escolha de região e starter em um save novo. |

### 6.4 Minijogos e batalha

| Arquivo | Função |
| --- | --- |
| `game_type_screen.h` / `.cpp` | Minijogo de tipos (efetividade). |
| `game_catch_screen.h` / `.cpp` | Minijogo de captura de selvagem. |
| `game_ball_screen.h` / `.cpp` | Minijogo com bola (quicar/acertar). |
| `game_memo_screen.h` / `.cpp` | Minijogo de memória (sequência de almofadas). |
| `game_clean_screen.h` / `.cpp` | Minijogo de limpeza. |
| `game_screen_common.h` / `.cpp` | Base comum aos minijogos (`isMinigame()`, placar, tempo). |
| `strength_screen.h` / `.cpp` | Saco de treino (força: ATK/DEF/SPE). |
| `battle_screen.h` / `.cpp` | Batalha contra selvagem: ataque, esquiva, descanso, contra-ataque e resultado. Usa `battle.h` para a lógica. |
| `dex_screen.h` / `.cpp` | Pokédex/galeria: grade, detalhe, filtros (região, capturados, shiny), ordenação. |

## 7. Pasta `tests/`, `tools/` e `web/`

Citadas no `README.md`, no `progress.md` e no código (ex.: `tests/Makefile`, `tools/build_web.sh`,
`web/index.html`, `web/manifest.json`), mas **ausentes neste checkout**:

- `tests/` – testes nativos (`make test`) para a lógica pura (`pet`, `battle`, `settings_logic`, etc.).
- `tools/` – geradores (`gen_dex.py`, `sprites.py`), empacotadores de sprites (`pack_pmd.py`,
  `pack_bundle.py`), miniaturas (`make_thumbs.py`) e envio ao SD (`send_sd.py`).
- `web/` – instalador web (Chrome/Edge, Web Serial). Páginas `index.html` (público),
  `dev.html` (debug, nunca publicar) e `release-gen3.html`.

## 8. Funcionalidades

- **Cuidado do pet**: alimentar (incluindo bagas por cor e doces), brincar, banho, luz,
  carinho e limpeza. Quatro necessidades decaem com o tempo: **FOOD, JOY, ENE e HYG**.
- **Ciclo de vida**: ovo (com cracks ao tocar), eclosão, idade, nível (60 min de jogo por nível),
  dormir e acordar.
- **Evolução**: ramificações, condições de dia/noite, vínculo (bond) e atributos. Exige nível e
  pelo menos três das quatro necessidades acima de 40. Decisão sempre apresentada ao jogador.
- **Personalidades** (5) influenciam ganhos e reações.
- **Pokédex/Box/Equipe**: registro de capturados e vistos, shiny, recompensas por marcos,
  moldura de coleção e *rank* de colecionador.
- **Batalhas**: contra selvagens (nível e espécie por fase do dia), turnos com ataque rápido,
  ataque pesado, esquiva, descanso e contra-ataque; recompensas de ATK/DEF/SPE.
- **Captura**: resultado de minijogo ou batalha; chance influenciada por nível, passos e shiny.
- **Minijogos**: tipos, captura, bola, memória e limpeza, com recordes.
- **Metas diárias** (cuidado, brincadeira, batalha, captura, memória) e **medalhas**
  (nível 10/25/50, bagas, sequência de 7 dias, vínculo, forma final, forma física, explorador de Hoenn).
- **Expedições**: 15/30/60 minutos, com itens no inventário (snack, energia, cuidado, treino),
  sítios de Hoenn e recompensa opcional por passos.
- **Passos**: contador diário e total, detecção por IMU com filtro de cadência (conta com tela
  apagada e com USB), recompensas diárias (500/2000/5000) e *ranks* de trilha.
- **Tempo real**: RTC para ciclo dia/noite e progressão offline (o pet "envelhece" enquanto desligado).
- **Áudio sintetizado**: efeitos e chirps gerados por código (sem samples originais).
- **Idiomas**: ES, EN, FR, DE, IT e PT (padrão definido em `i18n.h`).
- **Energia**: brilho automático, economia de energia, *light sleep*, desligar tela com PWR curto.
- **Salvamento**: NVS (`Preferences`) com gravação adiada quando a tela está apagada, escurecida
  ou o pet dorme. Relógio persistido com o save.
- **Sprites**: PMD/TPK2 (multi-ação, preferido) e TPK1 (monocromático, fallback), lidos do microSD.
- **Diagnóstico**: heartbeat `HEALTH` na serial e métricas de render/loop.

## 9. Convenções de código

- Constantes de jogo ficam em `#define` no topo de `pet.h` (tick, minutos por nível, duração de animações).
- Enums são usados para estados e identificadores (`PetMood`, `DailyGoalType`, `ExpeditionSite`, `Sfx`, `StrId`).
- Textos de UI passam por `T(StrId)`. A ordem do enum `StrId` deve coincidir com a tabela `STRINGS` de `i18n.cpp`.
- Telas herdam de `Screen` e são instâncias globais (ex.: `dexScreen`, `battleScreen`) ativadas com `setScreen()`.
- Lógica testável deve ficar em arquivos `.h` puros (ex.: `battle.h`, `settings_logic.h`, `dayphase.h`) e ser coberta por testes nativos antes de UI/hardware.
- Em UI, textos do firmware são majoritariamente ASCII (limitação da fonte): evitar acentos nas strings exibidas.
- Comentários são usados apenas quando precisam de esclarecimento.

## 10. Regras para mudanças

- Não editar `dex.h` e `species.h` manualmente.
- Atualizar `FW_VERSION` e, em release, os manifestos do instalador ao mesmo tempo.
- Não incluir comandos de depuração (`TESTMON`, `TESTEVO`, `dbgRunawayReady`) em builds públicos.
- Preservar saves: evitar mudanças de layout em NVS sem migração.
- Não incluir samples de jogos originais nem áudio protegido.
- Validação: testes nativos (`tests/`) e, para hardware, teste no dispositivo (tela, touch, IMU,
  som, SD e retenção de save), pois testes nativos não cobrem esses pontos.
