# Plano de implantação: mecanismo global de salvamento

## 1. Objetivo

Criar um mecanismo global de salvamento no NVS que centralize a flag `pendingSave` e grave o estado do jogo **no máximo a cada 5 minutos**. Hoje a flag existe só dentro de `Pet`, e a gravação depende de a tela estar apagada ou escurecida. O novo mecanismo passa a ser o único responsável por decidir quando gravar.

Requisitos definidos pelo usuário:

1. Um mecanismo global de salvamento, de uso comum a todo o app.
2. A flag `pendingSave` faz parte desse mecanismo.
3. A gravação ocorre somente a cada 5 minutos.

## 2. Situação atual

- `pendingSave` e `ticksSinceSave` ficam em `Pet` ([pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h), por volta da linha 300).
- Marcam `pendingSave = true` em vários pontos de [pet.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.cpp):
  - `Pet::tick()`, a cada 5 ticks (linhas 305 e 358). O tick roda a cada 60 s (`PET_TICK_MS` em [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h)).
  - Ações de jogo: carinho, passos, recompensas, `ensureStepDay` e outros (linhas 1309 a 1491).
- `Pet::save()` (linha 1781) zera `pendingSave` e `ticksSinceSave` e grava todo o estado, incluindo o `lastSeenEpoch` na chave `"seen"`.
- `Pet::flushSave()` (linha 378) só grava se `pendingSave` estiver ligada.
- A gravação pendente é disparada em [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp) somente quando `screenOff || dimStage >= 1 || pet.sleeping`. O motivo é que o NVS pode travar os dois núcleos por cerca de 1 s.
- Várias ações chamam `save()` ou `saveParty()` diretamente (por exemplo `chooseStarter` em [pet.h](C:/Users/cauea/Documents/projetos/TamaPokeExtended/pet.h)). Essas gravações acontecem na hora, sem passar pela flag.
- O relógio por software ([clock_service.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/core/clock_service.cpp)) grava na chave do namespace `tamapoke` a cada 60 s, mas só sem RTC válido. Ele usa `SAVE_INTERVAL_MS` próprio.
- Não existe um ponto central de salvamento. Cada parte decide quando gravar.

## 3. Arquitetura proposta

### 3.1 Nova classe `SaveManager`

Arquivo: `src/core/save_manager.h` e `src/core/save_manager.cpp`, seguindo o padrão de `clock_service.h`.

Campos:

- `pendingSave` (bool): há estado alterado aguardando gravação.
- `lastWriteMs` (uint32): instante da última gravação, em `millis()`.
- Ponteiro ou callback registrado para o `Pet::save()`. A classe não conhece `Pet` diretamente, para continuar genérica.

Métodos:

- `markDirty()`: liga `pendingSave`. Não grava.
- `update(nowMs)`: se `pendingSave` e `nowMs - lastWriteMs >= SAVE_INTERVAL_MS` (300 000 ms), chama o callback de gravação e limpa `pendingSave`. Chamado a cada loop.
- `flushNow()`: grava na hora, ignorando o intervalo. Usado somente pela pressão longa do PWR (seção 3.5b).
- `setInterval(ms)`: altera o intervalo mínimo entre gravações. Usado pela regra de bateria fraca (seção 3.5).
- `isPending()`: somente leitura.

### 3.2 Regra de intervalo

- A primeira gravação depois de um boot pode ocorrer a qualquer momento, pois `lastWriteMs` começa em 0 e a janela já passou. Assim, um save pendente logo após ligar não fica preso.
- Depois disso, a janela é de exatamente 5 minutos a partir da última gravação, independente de a tela estar acesa ou apagada.

### 3.3 Integração

- `Pet` deixa de decidir quando gravar. Ele apenas chama `saveManager.markDirty()` onde hoje define `pendingSave = true`.
- `ticksSinceSave` sai de `Pet`. A contagem de 5 ticks deixa de existir, porque o intervalo passa a ser temporal.
- `Pet::flushSave()` e `Pet::savePending()` são removidos, substituídos por `SaveManager`.
- [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp) passa a chamar `saveManager.update(now)` a cada loop. O bloco atual de `flushSave` por tela apagada é removido.
- `saveParty()` e as gravações diretas de `save()` (como `chooseStarter`) passam a usar `markDirty()`, sem exceção. Assim, todas as gravações respeitam a janela de 5 minutos.
- Ações críticas (alimentar, starter, evolução e liberação de pet) também usam `markDirty()` e esperam os 5 minutos.
- O relógio por software passa a usar o `SaveManager`, com a mesma janela de 5 minutos. Ele deixa de ter `SAVE_INTERVAL_MS` próprio.

### 3.4 Persistência

- Chaves NVS atuais não mudam. Não há migração de save.
- O callback de gravação chama `Pet::save()`, que grava o estado completo do pet.

### 3.5 Gatilhos de gravação antecipada

**a) Bateria fraca (intervalo de 30 s)**

- Leitura: `batPercent()`, `batCharging()` e `usbPresent()` em [power.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/hardware/power.cpp). Já têm cache de 2 s, então não geram I2C a cada loop.
- Limite: **10%**. Abaixo disso, e sem USB, o intervalo do `SaveManager` passa a ser **30 s**.
- Histerese: a volta para 5 min só ocorre acima de **12%**, para evitar alternância perto do limite.
- Carregando ou com USB conectado: o intervalo fica em **5 min**, mesmo que o nível esteja baixo.
- Chamada: `saveManager.setInterval(...)` em `app_controller.cpp`, a partir do estado lido pelas funções acima.
- Sem bateria instalada (`batPercent() == -1`): intervalo de 5 min.

**b) Botão PWR pressionado (gravação na pressão longa)**

- Hoje `pwrSetup()` em [power.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/hardware/power.cpp) habilita só `XPOWERS_AXP2101_PKEY_SHORT_IRQ`, e o desligamento por hardware ocorre após 4 s (`XPOWERS_POWEROFF_4S`).
- Mudança: habilitar também `XPOWERS_AXP2101_PKEY_LONG_IRQ`, que dispara antes do corte de 4 s.
- Ao detectar a interrupção (`isPekeyLongPressIrq()`), o loop chama `saveManager.flushNow()`.
- A pressão curta não aciona gravação. Ela continua só alternando o display.
- A gravação precisa caber na janela de 4 s, então o tempo de dispara precisa ser medido no hardware.
- Limite: cobre só o desligamento pelo botão. Não protege contra remoção da bateria, queda de tensão ou travamento.

## 4. Fases

### Fase 1: Classe `SaveManager` e testes
- Criar `src/core/save_manager.h` e `.cpp` com `markDirty`, `update`, `flushNow` e `isPending`.
- Testes nativos em `tests/` cobrindo: não grava sem flag suja; primeira gravação após boot; não grava antes de 5 minutos; grava exatamente ao completar 5 minutos; `flushNow` ignora o intervalo; múltiplas marcas sujas dentro da janela geram uma única gravação.

### Fase 2: Integração com `Pet`
- Substituir `pendingSave = true` por `markDirty()` nos pontos listados na seção 2.
- Remover `ticksSinceSave`, `savePending` e `flushSave` de `Pet`.
- Manter `Pet::save()` como a operação de gravação, chamada pelo `SaveManager`.

### Fase 3: Loop do app
- Chamar `saveManager.update(now)` em [app_controller.cpp](C:/Users/cauea/Documents/projetos/TamaPokeExtended/src/app_controller.cpp).
- Remover a condição `screenOff || dimStage >= 1 || pet.sleeping` do flush.
- Migrar `saveParty()`, as gravações diretas de `save()` e o relógio por software para `markDirty()` / `SaveManager`.

### Fase 4: Validação no hardware
- Medir o tempo de travamento do NVS em gravação com tela acesa, para confirmar se a remoção da condição de tela apagada é segura.
- Medir se a janela de 5 minutos mantém o pet responsivo durante interações.

### Fase 5: Gatilhos de bateria e PWR
- Implementar `setInterval` e a regra de bateria (limite 10%, histerese 12%, USB mantém 5 min) em `app_controller.cpp`.
- Habilitar `XPOWERS_AXP2101_PKEY_LONG_IRQ` em `pwrSetup()` e chamar `flushNow()` ao detectar a pressão longa.
- Medir no hardware o tempo entre a interrupção de pressão longa e o corte de 4 s, e confirmar que a gravação cabe nessa janela.
- Confirmar no datasheet do AXP2101 o tempo de disparo da pressão longa.

### Fase 6: Documentação
- Atualizar [ARQUITETURA.md](C:/Users/cauea/Documents/projetos/TamaPokeExtended/ARQUITETURA.md) com a descrição de `SaveManager` e da regra de 5 minutos, no mesmo lugar onde o relógio é descrito.
- Atualizar [PlanoImplantacao/steps_centralizacao.md](C:/Users/cauea/Documents/projetos/TamaPokeExtended/PlanoImplantacao/steps_centralizacao.md) se a persistência dos passos mudar, pois ele descreve gravação a cada 100 passos.

## 5. Testes

Testes nativos, cobrindo só o comportamento novo:

- Nada é gravado sem `markDirty`.
- Uma marca suja grava somente após 5 minutos.
- Várias marcas dentro da janela resultam em uma gravação.
- `flushNow` grava mesmo dentro da janela.
- Ação crítica (alimentar, starter, evolução, liberação) não grava antes de 5 minutos.
- Pressão curta do PWR não aciona gravação.
- Com bateria abaixo de 10% e sem USB, uma marca suja grava após 30 s, não 5 min.
- Bateria voltando a 12% ou mais restaura o intervalo de 5 min (histerese).
- Com USB conectado, o intervalo permanece em 5 min mesmo com bateria abaixo de 10%.
- Sem bateria instalada, o intervalo permanece em 5 min.

Checklist no hardware:

- Interagir com o pet por 10 minutos com a tela acesa: a gravação ocorre a cada ~5 minutos, sem travamentos perceptíveis.
- Desligar logo após uma ação: perda de no máximo 5 minutos de progresso, conforme o risco da seção 7.
- Ligar o pet e confirmar que o estado salvo é carregado corretamente.

## 6. Fora de escopo

- Mudança no formato do save ou nas chaves NVS.
- Mudança na lógica de simulação do pet (fome, energia, passos etc.).
- Alteração da frequência do tick do pet (60 s).

## 7. Riscos

- **Perda de progresso ao desligar**: até 5 minutos de estado podem se perder se o dispositivo for desligado sem gravação. A exceção é o desligamento pela pressão longa do PWR (seção 3.5b). Remoção da bateria, queda de tensão e travamentos continuam sem cobertura. A gravação com tela apagada deixa de existir, o que aumenta essa janela na prática.
- **Travamento do NVS com tela acesa**: o motivo original do flush apenas com tela apagada é o travamento de cerca de 1 s. Com gravação a cada 5 minutos, o custo cai, mas precisa ser medido (fase 4).
- **Gravações diretas existentes**: `save()` e `saveParty()` chamados diretamente gravam sem respeitar a janela se algum caminho for esquecido na migração. Isso anula a regra de 5 minutos, por isso a fase 2 deve listar todos os pontos.
- **Relógio com `millis()`**: `millis()` reinicia no boot. A janela é relativa ao boot, o que é suficiente para o objetivo.
- **Gravação a cada 30 s com bateria fraca**: cada gravação trava os dois núcleos por cerca de 1 s. Isso ocupa cerca de 3% do tempo, aceitável só enquanto a bateria está fraca.
- **Corte por tensão baixa antes de 10%**: se o AXP2101 desligar por tensão antes desse nível, a regra de bateria não tem efeito. O limite deve ficar acima do ponto de corte, a confirmar no hardware.
- **Medição da bateria**: a porcentagem do fuel gauge pode ser imprecisa. Comparar com a tensão medida antes de confiar no limite.
- **Pressão longa do PWR**: a interrupção pode disparar com latência de um frame se o loop estiver ocupado com render.

## 8. Decisões confirmadas

1. **Eventos críticos**: alimentar, starter, evolução e liberação de pet esperam os 5 minutos. Não há gravação imediata.
2. **Flush com tela apagada**: removido. A gravação ocorre só pela janela de 5 minutos e pela pressão longa do PWR.
3. **Relógio por software**: passa a usar o `SaveManager`.
4. **`saveParty()`**: respeita a janela de 5 minutos.
5. **Pressão curta do PWR**: não grava. Só a pressão longa aciona `flushNow()`.
6. **Limite de bateria fraca**: 10%, com histerese de 12%. Com USB conectado, o intervalo permanece em 5 minutos (seção 3.5a).
