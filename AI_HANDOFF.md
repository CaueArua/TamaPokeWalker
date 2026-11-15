# TamaPoke Expanded - AI Handoff

Status: 2026-11-05 (versões conferidas; cronologia até 1.36.1 e o candidato local 1.36.2 incluídos)

## Projeto e hardware

Este repositório é um firmware muito ampliado e voltado à comunidade para
o Waveshare ESP32-S3 Touch AMOLED 1.75. É um fork de
`socquique/TamaPoke` e é mantido como variante "Expanded".

- Ponto de entrada do firmware: `TamaPokeExtended.ino`
- Estado inicial do firmware: `1.32.1-caught-mark`
- Build de teste local: `1.36.2-hoenn-walk-local` (via `TAMAPOKE_LOCAL_TEST`;
  expedições Hoenn com metas de passos, veja `HOENN_WALK_LOCAL.md`)
- Release público Gen-3: `1.36.2` (via
  `TAMAPOKE_GEN3_RELEASE`, sem comandos de depuração locais)
- Placa: ESP32-S3, 16 MB de flash, OPI PSRAM, AMOLED redondo 466x466, touch CST9217,
  RTC PCF85063, PMU AXP2101, áudio ES8311
- Toolchain validada: ESP32-Core 3.3.10, Arduino_GFX 1.6.7,
  SensorLib 0.4.1 e XPowersLib 0.3.3.
- Idioma: DE, EN, ES, FR, IT, PT. Código e textos de UI usam, por causa da
  fonte, principalmente ASCII sem acentos/umlauts.

O firmware guarda o save do jogo na NVS do ESP32. Em uma atualização, o
instalador **não** pode ser executado com `Erase device`, senão o save
é apagado.

## Módulos do firmware

- `TamaPokeExtended.ino` contém a entrada do Arduino (`setup()`/`loop()`).
  `src/hardware_setup.cpp`, `src/sensor_setup.cpp` e
  `src/program_setup.cpp` encapsulam a inicialização de hardware, sensores e programa.
- `src/app_controller.*` executa as atualizações periódicas de pet, sensores, energia e
  renderização.
- `src/serial_console.*` processa comandos seriais, backups de save e
  diagnósticos.
- `src/ui/screen_controller.*`, `touch_controller.*` e `power_manager.*`
  encapsulam roteamento de telas, gestos de touch e gerenciamento de energia.
- `src/ui/render_utils.*` e `scene_renderer.*` contêm funções de desenho e cena
  compartilhadas; cada tela fica em seus próprios arquivos `*_screen.*`.
- Parte do estado ainda é compartilhada entre módulos. A
  migração gradual desse estado ainda está pendente.

## Regra de trabalho importante

**Nunca fazer push para o GitHub, criar releases ou atualizar a PR sem a
solicitação explícita do usuário.**

O usuário quer testar versões locais inacabadas primeiro no hardware dele. Build
local, testes e o instalador web local são sempre permitidos e desejados.

## Estado atual do Gen-3

- Branch `local/full-gen3`; a tag `local-before-gen3` permite voltar rapidamente ao
  estado Gen-2 que funciona. O estado Gen-3 testado `da8bfa8` já está em `fork/main`. O
  instalador hospedado e o release do GitHub usam o estado Gen-3 `1.36.2`, sem depuração.
- Dex local #1–386 com seis idiomas, caminhos de Dex uint16, stats/tipos Gen-3,
  regras de evolução e o gate de stats 3-de-4 acima de 40 por cento.
- Primeira inicialização local: escolher a região Kanto/Johto/Hoenn e depois um dos três
  iniciais da região. Saves existentes não são migrados nem apagados.
- Todos os 270 arquivos Gen-3 do SpriteCollab (normal + shiny) estão em
  `tools/sdcard/mons/`; `thumbs.bin` tem 386 entradas.
- `web/sprites-gen3-update.pak` cobre #252–386 (cerca de 38 MB). O
  `web/sprites.pak` público continua com #1–251. O botão local do pacote completo de 386
  envia os dois arquivos em sequência; assim, cada arquivo do GitHub fica abaixo do
  limite de 100 MB.
- Um único arquivo `sprites-gen3-full.pak` não existe mais e não pode ser exigido em
  instruções ou textos de release.
- Teste web local: `cd web && python3 -m http.server 8000`, depois
  `http://localhost:8000/dev.html` no Chrome/Edge. Diagnósticos de passos, IMU e
  evolução funcionam ali por cliques; não é preciso terminal `screen`.
- Build com `bash tools/build_web_local.sh`; as quatro partes do firmware local ficam
  depois em `web/firmware/tamapoke-1.36.2-hoenn-walk-local-*.bin`.
- O release público, sem depuração, é gerado com `bash tools/build_web.sh`.
  `bash tools/build_web_gen3.sh` é o build Gen-3 subjacente;
  `web/release-gen3.html` continua sendo uma página local de referência.
- README, instalador e texto de release deixam claro: só gravar o firmware não basta;
  depois disso o passo 2 para os sprites na microSD é obrigatório. Com cartão novo, vazio
  ou incerto, use sempre o pacote completo de 386 em vez de só a atualização Gen-3.

## Árvore de trabalho atual

O instalador público aponta para o build Gen-3 sem depuração `1.36.2`.
Para testes de hardware continuam existindo `web/dev.html` e
`web/manifest-local.json`; esses arquivos de depuração propositalmente não fazem parte
do fluxo de instalação público.

Guards atuais de teste local em `TamaPoke.ino`:

- `TAMAPOKE_LOCAL_TEST` define a versão exibida como `1.36.2-hoenn-walk-local`.
- `TAMAPOKE_GEN3_RELEASE` define a versão exibida como `1.36.2` e libera o Dex de 386,
  mas não libera comandos de depuração.
- Os comandos seriais `TESTMON`, `TESTEVO`, `CAUGHT`, `CAUGHT <dex>`, `BATTLE`
  e `BATTLE <dex>` só ficam ativos neste build de teste.
- Essa versão de teste não pode ser acidentalmente adicionada a `web/index.html` ou
  `web/manifest.json` nem ativada como instalador público do GitHub Pages; ela fica em
  `web/dev.html` e `web/manifest-local.json` para testes de hardware.
- `web/release-gen3.html` e `web/manifest-gen3.json` são a prévia pública local
  sem interface de depuração. O único botão de sprites envia `sprites.pak` e
  `sprites-gen3-update.pak` obrigatoriamente em sequência.


Estado do Git ao criar este arquivo:

- Branch atual: `local/full-gen3` (o estado Gen-2 permanece no histórico em
  `local/full-gen2`).
- `fork`: `https://github.com/ShadowEnemyx/TamaPoke.git`
- `origin`: `https://github.com/socquique/TamaPoke.git`
- O ponto de partida está salvo localmente como `3b24ea8` (`chore: save local baseline before gen2 watermark`)
  e também com a tag `local-baseline-before-gen2-watermark`.
- O estado de funcionalidades local está salvo como `feat: add gen2 starters and local watermark`;
  o estado final foi publicado em `fork/main` após autorização explícita do usuário.
- O último estado Gen-2 testado está marcado historicamente com a tag
  `local-full-gen2-final`. O branch de trabalho atual é
  `local/full-gen3`; antes das correções de bugs existe também a tag local
  `local-before-gen3-bugfix-20260825`.
- A árvore de trabalho contém o estado publicado de Gen-2, marca d'água, localização,
  sprites e contagem de passos. `web/index.html` e
  `web/manifest.json` apontam para o build público; só `web/dev.html`
  e `web/manifest-local.json` ativam os comandos de depuração.
- O autor principal deixou a grande PR do Expanded aberta e linka o fork,
  mas não quer fazer merge completo no projeto base por causa do escopo, da marca e
  do histórico de binários. PRs pequenas e posteriores seriam possíveis, mas só
  por decisão do usuário.

## Funções existentes

### Jogo principal

- Ovo, escolha de inicial, eclosão, cuidados, sono, higiene, fome, alegria,
  energia, peso, vínculo, apelidos, evolução, despedida, soltura e fuga.
- Idade e cuidados continuam a correr via RTC mesmo com o display desligado;
  o progresso offline é limitado.
- Pokédex/Galeria diferencia Pokémon criados (`raised`) e capturados (`caught`).
- Perfil, personalidade, metas diárias, medalhas, progresso e
  rank de colecionador são acessíveis em oito cartões com swipe.


### Batalha

- Batalhas selvagens manuais pela carta de Batalha, além de prompts raros e opcionais de encontro selvagem na tela principal.
- Os níveis dos oponentes ficam normalmente perto do nível do pet, mas podem ficar alguns níveis abaixo. Lendários selvagens não são previstos na v1.
- Os tipos aparecem nas lutas e influenciam o dano de forma moderada.
- Ações: Quick, Heavy, Dodge/Counter, Rest e Run.
  - Quick: menos dano, mais seguro e reduz o dano recebido.
  - Heavy: mais dano, mas o oponente pode desviar/contra-atacar.
  - Dodge: pode preparar um contra-ataque para o próximo golpe.
  - Rest: no máximo duas vezes por luta; cura e concede Guard.
- Sem morte de Pokémon, sem captura em caso de derrota. Vitórias/derrotas/sequências são persistentes e dão recompensas moderadas de treino.
- Após uma vitória há exatamente uma tentativa opcional de captura. Em uma derrota apertada pode haver uma pequena chance de "respect catch".
- Durante a luta, uma pequena Pokébola ao lado direito do nome do oponente indica se o Pokémon adversário já foi capturado. O símbolo usa apenas o bitmap existente `dexCaught` e não altera a estrutura do save.

### Minigames e eventos

- Menu de brincadeira com Ball, Catch, Memo, Clean e Type.
- Ball é um jogo de reação intenso com movimento rápido/imprevisível.
- Catch é um jogo de reação com janelas de tempo cada vez mais curtas.
- Memo mostra uma sequência de cores que deve ser repetida; a instrução fica visualmente entre as fileiras de pads e os pads têm sons próprios.
- Clean e Type complementam o treino de higiene e de tipo/ataque.
- Eventos raros e opcionais do pet (Berry, Heart, Sparkle) e encontros selvagens opcionais raros não interrompem outros modais da UI.

### Expedição e inventário

- Oitava carta `EXPEDITION` com excursões de 15/30/60 minutos.
- Um pequeno chip na tela principal aparece apenas com excursão ativa, achado disponível ou itens existentes; um toque abre diretamente a carta de Expedição.
- As excursões continuam via RTC, mesmo com a placa desligada.
- A recompensa é definida no início e salva de forma persistente.
- Inventário: Trail Snack, Energy Tonic, Care Kit e Train Token. No máximo três de cada item; o Train Token abre a escolha de ATK/DEF/SPD.
- O chip da tela principal abre diretamente a carta 8 e mostra o tempo restante da excursão, o achado disponível ou a quantidade do inventário.

### Configurações, ajuda e desempenho

- Configurações: deslizar para baixo abre duas páginas. Som (Off/Médio/Máx) e Power Save
  valem imediatamente; brilho e Wi-Fi são apenas prévia. A segunda página abre
  data/hora; Sistema e Wi-Fi ainda não estão disponíveis.
  Escolha de idioma e Ajuda não são acessíveis por este menu no momento.
- Um modo de som baixo já salvo é mantido até o controle de som ser usado; depois ele usa Off/Médio/Máx.
- Power Save vem desativado por padrão; reduz a renderização em idle e usa Light Sleep
  apenas com escurecimento/tela desligada. Cuidados, RTC e wake por touch continuam
  ativos.
- Oito páginas de ajuda integradas descrevem operação, batalha e sistemas.
- Telas estáticas usam dirty rendering; jogos/batalhas ativos continuam sendo renderizados regularmente.


### Movimento, fase do dia e evolução

- `dayphase.h` fornece uma fase compartilhada de manhã/dia/tarde/noite para o fundo, o pool de selvagens e o consumo de cuidados.
- À noite, a FOOD cai mais devagar com o pet acordado; o sono permanece igual.
- Sacudir com o QMI8658 concede JOY de forma limitada. O pedômetro conta passos mesmo com a tela desligada; contadores diários/totais, recompensas de trilha e bônus de shiny/captura selvagem
  ficam na NVS. O contador de software atual usa impulsos de aceleração positivos filtrados e conta também via USB, assim que dois passos rítmicos são confirmados. O pedômetro bruto do QMI continua visível como diagnóstico.
- A evolução nunca é executada automaticamente: o CTA vermelho fica visível quando o nível é atingido e lembra quando os valores de cuidado estão baixos. A opção de manter a forma é oferecida de novo no nível 100, após um dia de jogo.

## Áudio

O áudio é totalmente sintetizado via ES8311/I2S. Não são usados arquivos originais de áudio Pokémon nem sons de ROM.

- `SOUND_FULL` / TON VIEL: UI, minigames, batalha, ambiente e chamados do pet.
- `SOUND_MED`: sons importantes de cuidado, batalha, evento e resultado.
- `SOUND_LOW`: grandes eventos e alertas.
- `SOUND_OFF`: mudo.

Os chamados das espécies e todos os sons de UI/batalha/minigames são sintetizados:

- quatro tons em vez de três, mais longos e sem onda de ruído
- volume de síntese mais alto
- em TON VIEL, o toque no pet tem prioridade sobre o clique genérico
- toque no pet, detalhe da galeria e batalha selvagem têm caminhos próprios de chamado/SFX
- os modos de som continuam separados: VIEL, MITTEL, WENIG, AUS

Volume e efeito ainda precisam ser avaliados no hardware real; builds para desktop não substituem o caminho ES8311/alto-falante.


## Cronologia de versões

Os itens a seguir documentam o estado de desenvolvimento da época. Nem toda
versão de teste foi publicada; informações sobre branches antigos não descrevem
a árvore de trabalho atual. `1.36.2-hoenn-walk-local` é o
estado local de depuração atual; `1.36.2` é o release Gen-3 público, sem depuração.

1. `1.29.3-reliability`
   - Reset dos bloqueios de interação/evolução/despedida ao iniciar um novo pet.
   - Despedida/soltura/fuga é concluída com segurança após um reset.
   - O nível do pet é limitado a 100.
   - Os deadlines de `millis()` são seguros contra overflow.
   - Os cartões voltam a ser atualizados nos ticks do pet.
   - Textos literais de UI restantes foram associados a chaves i18n.
   - Arquivos auxiliares `._*` do macOS dentro de `.git` foram removidos; `git fsck`
     ficou limpo depois, exceto por um commit dangling normal.
2. `1.29.4-profile-tap`
   - Os círculos verdes no perfil eram molduras de colecionador. Eles não são mais
     desenhados sobre o retrato.
   - Um toque no retrato do perfil agora toca o chamado da espécie.
3. `1.29.5-pet-chirps`
   - Chamados de espécie sintetizados, mais claros e audíveis.
4. `1.30-expedition-hud`
   - Entrada visível e adequada a tela redonda para excursão e inventário na
     tela principal. O chip mostra o tempo restante da excursão, o achado disponível ou a
     quantidade do inventário e fica oculto em todas as visões modais.
5. `1.30.1-caught-frames`
   - Batalhas selvagens mostram uma pequena Pokébola à esquerda da barra de HP do oponente
     quando esse Pokémon já foi capturado.
   - As molduras de perfil voltaram a aparecer: seis variantes de canto/destaque ficam
     atrás do retrato e não cobrem o sprite.
6. `1.31.0-day-imu`
   - API compartilhada de fase do dia em `dayphase.h` (manhã/dia/tarde/noite).
   - Acordado à noite: FOOD -1/tick em vez de -2; sono inalterado. Os minutos offline
     usam a hora de cada minuto.
   - Pool de selvagens por fase: manhã Grass/Flying/Normal/Bug, tarde Water/Flying/Fire,
     noite Ghost/Poison/Bug. O dia permanece sem filtro. Sem lendários.
   - O idle noturno roda com menos frequência. Uma sobreposição matinal uma vez por dia de calendário,
     sem acordar o pet automaticamente.
   - QMI8658: sacudir dá JOY na tela principal (cooldown de 25 s, 8/dia).
     Passos também com a tela desligada; a build da época descartava passos via USB.
     A build de teste de passos posterior contava também via USB. Serial: `IMU`, `SHAKE`,
     `WALK n`, `STEPS`, `STATS`. A página de teste local oferece também os botões
     clicáveis de diagnóstico `IMU`/`STATS`; `STATS` mostra também `usb=0/1`.
7. `1.31.1-shake-fix`
   - Limiar de sacudir reduzido (~1,45 g ou giroscópio), acelerômetro+giroscópio, ambos os endereços I2C.
8. `1.32.0-evo-cta`
   - O botão vermelho de evolução permanece quando o nível é atingido, mesmo com uma barra abaixo de 40. Um toque então lembra das barras.
   - "Manter forma" no nível 100 volta a ser oferecido após um dia de jogo.
9. `1.32.1-caught-mark`
   - Estado público do instalador com marcador de captura na batalha selvagem.
   - O marcador fica ao lado do nome do oponente e mostra apenas Pokémon selvagens já capturados.
10. `1.32.1-local-test`
    - Variante de teste local com comandos seriais extras para o marcador de captura e
      oponentes de batalha forçados. Não publicar.
11. `1.34.0-gen2-full-local`
    - Dex completo #1-251 com nomes, valores, tipos e seis idiomas da Gen-2 e 122 regras de evolução,
      incluindo as ramificações de Eevee, bebês e Tyrogue.
    - Uma evolução exige estritamente três de quatro valores de cuidado acima de 40;
      vínculo, dia/noite e proporções de stats são verificados para as regras correspondentes.
    - Arquivos SpriteCollab normal/shiny e miniaturas de #161-251 estão empacotados localmente;
      `sprites-gen2-update.pak` é uma atualização incremental.
    - `web/dev.html` oferece seleção e botões de teste para as 251 espécies sem terminal; os comandos de firmware
      ficam ativos apenas no build de teste local.
12. `1.35.3-soft-step-local`
    - Contador persistente de passos diários/totais com recompensas de trilha de 500/2.000/5.000 e marcos de rank de trilha.
    - Passos diários melhoram a chance de shiny selvagem e a chance de captura;
      batalhas selvagens shiny são exibidas e salvas no registro shiny do Dex.
    - O contador de hardware QMI continua visível para diagnóstico; os passos são reconhecidos a partir de
      impulsos de aceleração filtrados. USB não bloqueia mais a contagem; status bruto, de software e USB
      podem ser verificados por `STATS`/`IMU`.
13. `1.36.0-gen3-local`
    - Dex local e acervo de sprites #1-386 com iniciais de Hoenn, todas as espécies Gen-3,
      seis idiomas e regras de evolução Gen-3.
    - Somente `tools/build_web_local.sh` gera este candidato. O build público Gen-2
      permanece inalterado e é gerado a partir de um commit fixado.
14. Release `1.36.0`
    - Perfil de build público Gen-3 com 386 espécies, mas sem comandos de teste locais.
    - A etapa de sprites envia o pacote Gen-1/2 e o pacote Gen-3 em sequência.
    - Ativado no GitHub Pages e publicado como release do GitHub.
15. `1.36.1` (público)
    - Backup e restauração completos do save por USB no navegador, com backup de segurança automático
      (commit `release: publish save backup update v1.36.1`).
    - Antes disso, estado público anterior do instalador (`web/index.html`).
16. `1.36.2` (público) / `1.36.2-hoenn-walk-local` (depuração)
    - Locais de expedição de Hoenn e metas de passos 250/500/1.000 para 15/30/60 minutos
      com item extra opcional; o backup do save versão 2 inclui o progresso da caminhada.
    - A variante de depuração fica local; as funções estão incluídas no `1.36.2` público. Segundo o dono (estado de 2026-11-06), está há semanas em uso contínuo no hardware
      sem anomalias (`HOENN_WALK_LOCAL.md`).

## Arquitetura e arquivos importantes


- `TamaPoke.ino`: UI, roteamento de touch, renderização, cartões, minigames, tela de batalha,
  configurações, ajuda, estado de runtime local, leitura do IMU e sobreposição matinal.
- `dayphase.h`: hora, fase e noite visual a partir da época do RTC.
- `imu.h` / `imu.cpp`: leitura do QMI8658, borda de sacudida, delta do pedômetro.
- `pet.h` / `pet.cpp`: modelo persistente do pet, save/load na NVS, cuidados,
  progressão, Pokédex, captura, eventos, expedição e itens.
- `battle.h` / `battle.cpp`: lógica de batalha testável e multiplicadores de tipo.
- `audio.h` / `audio.cpp`: fila de som, ES8311, SFX e chamados das espécies.
- `species_chirp.h` / `species_chirp.cpp`: perfis de síntese gerados por número do Dex,
  juridicamente seguros.
- `i18n.h` / `i18n.cpp`: todos os textos visíveis da UI e nomes do Dex.
- `time_utils.h`: `deadlineActive`, `deadlineReached` e `deadlineRemaining`, seguros contra overflow, para timers de `millis()`.
- `tests/pet_tests.cpp` e `tests/battle_tests.cpp`: testes nativos de regressão.
- `tools/build_web.sh`: gera as quatro binárias separadas do instalador público Gen-3.
- `tools/build_web_gen3.sh`: gera e valida o release `1.36.2` sem depuração,
  sem usar o build local de depuração.
- `web/manifest.json` e `web/index.html`: instalador web. As partes são gravadas
  separadamente para que NVS/save sejam mantidos nas atualizações.
- `web/dev.html` e `web/manifest-local.json`: instalador local de teste, não usado no
  fluxo público do GitHub Pages.
- `web/release-gen3.html` e `web/manifest-gen3.json`: prévia local do instalador
  público Gen-3, sem botões de teste.


## Testes e gravação local

Execute na pasta do repositório (`TamaPoke`):

```bash
cd tests && make clean && make test
cd .. && bash tools/build_web.sh
python3 -m http.server 8000 --directory web
```

Depois abra no Mac o Chrome ou o Edge:

```text
http://127.0.0.1:8000/?v=1.36.2
```

Conecte o Waveshare por USB e, no instalador, **mantenha Erase device desativado**.
O servidor local precisa continuar rodando durante a gravação. O instalador web atual
espera estes quatro arquivos:

```text
web/firmware/tamapoke-1.36.2-bootloader.bin
web/firmware/tamapoke-1.36.2-partitions.bin
web/firmware/tamapoke-1.36.2-boot_app0.bin
web/firmware/tamapoke-1.36.2-app.bin
```

### Instalador local de teste

Para testes de hardware usa-se o caminho de build local protegido:

```bash
bash tools/build_web_local.sh
# ou sem alterar arquivos existentes do instalador:
bash tools/build_web_local.sh --check
```

As quatro partes geradas precisam ficar com os nomes `1.36.2-hoenn-walk-local` citados em
`web/manifest-local.json`. As partes do instalador web são versionadas para testes locais
e públicos reproduzíveis. Depois:

```bash
python3 -m http.server 8000 --directory web
```

Abra `http://127.0.0.1:8000/dev.html` no navegador. Também na atualização de teste
local, mantenha `Erase device` desativado.

### Referência pública do release Gen-3

```bash
bash tools/build_web_gen3.sh
python3 -m http.server 8000 --directory web
```

Abra `http://127.0.0.1:8000/release-gen3.html` no navegador. Esta página não tem
comandos de depuração e oferece exatamente um botão obrigatório de sprites completo.
A página pública `web/index.html` segue o mesmo fluxo normal.

## Verificação antes de uma eventual publicação

1. `cd tests && make clean && make test`
2. `bash tools/build_web.sh --check`, `bash tools/build_web_local.sh --check`
   e `bash tools/build_web_gen3.sh --check`
3. `git diff --check`
4. Teste de fumaça no hardware sem Erase:
   - Save/pet permanece.
   - Tocar o pet na tela principal várias vezes: o chamado precisa ser claramente audível.
   - Abrir o perfil: alternar as seis molduras de canto/destaque visíveis; testar tocar no retrato.
   - Verificar rapidamente Batalha, menu de brincadeiras, Memo, Expedição, Sono, wake por PWR e Configurações.
   - No build de teste local, verificar `TESTMON`, `TESTEVO`, `CAUGHT <dex>` e `BATTLE <dex>`; no
     build público esses comandos de depuração estão intencionalmente ausentes.
5. Só depois de autorização explícita do usuário: commit/push/publicação.

## Temas em aberto e próximos passos seguros

- O dono confirmou em hardware a gravação local, os sprites Gen-3 e a contagem real de passos
  em princípio. Para a liberação do release ainda falta o teste ampliado: serial `IMU`, `STEPS`,
  `WALK n`, passos por software com a tela desligada e via USB, recompensas de trilha,
  captura shiny selvagem, FOOD noturna, sobreposição matinal e
  sono que continua ao sacudir. Ajustar os limiares depois da sessão.
- Verificar visual/acusticamente, no Waveshare real, os chamados sintéticos dos pets e as seis molduras de perfil.
- Após novo feedback do usuário, fazer novas caçadas de bugs focadas. Em revisões, verificar
  sempre primeiro erros reais, riscos para o save e hitboxes de touch.
- Verificar em todas as mudanças de UI na tela redonda se há texto cortado e entradas indevidas.
- Nenhuma nova grande função de batalha sem decisão do usuário; antes confirmar no hardware
  marcador, molduras, hitboxes de touch e segurança do save.
- Não voltar a adicionar em massa binários históricos de firmware ao Git. Publicar apenas
  as quatro partes atuais do instalador de uma versão liberada, de forma direcionada.

