# TamaPoke – Regras do projeto para sessões de IA

Firmware ESP32-S3 (Arduino) para o Waveshare ESP32-S3-Touch-AMOLED-1.75, mais o instalador web em `web/`.
Pasta de trabalho: `/Volumes/2TB SSD/Backup bot/Apps/tamapoke/TamaPoke` (branch `local/full-gen3`).

## Ordem de leitura

1. `PROJEKT_KONTEXT.md` – tecnologia permanente, arquivos, testes, regras de trabalho (resumido).
2. `AI_HANDOFF.md` – estado detalhado, histórico de versões, checklist de verificação.
3. `progress.md` – progresso atual e caminhos do instalador.
4. `HOENN_WALK_LOCAL.md` – apenas ao trabalhar no candidato local `1.36.2-hoenn-walk-local`.
5. `web/README.md`, `tools/assets/README.md` – apenas para trabalho com o instalador ou com sprites.

As versões vinculantes estão no código: `FW_VERSION` em `TamaPoke.ino`, `tools/build_web_gen3.sh`,
`tools/build_web_local.sh` e nos manifestos em `web/`. Em caso de divergência vale o código; depois, alinhar a documentação.

## Regras rígidas

- **Nunca fazer push, release, alterar o GitHub Pages ou alterar o PR sem solicitação explícita.**
  Build, testes e o instalador web local são sempre permitidos.
- Em atualizações de firmware, **deixar "Erase device" desmarcado**, senão o progresso do jogo (NVS do ESP32) é perdido.
- Saves/NVS não podem ser postos em risco por migrações desnecessárias. Nova lógica primeiro é coberta por testes nativos, depois UI/hardware.
- A instalação Gen 3 tem sempre duas etapas: gravar o firmware **e** enviar os dois pacotes de sprites (`sprites.pak`,
  `sprites-gen3-update.pak`) para o microSD. `sprites-gen3-full.pak` não existe e não pode ser exigido em lugar nenhum.
- `web/dev.html`, `web/manifest-local.json` e `TAMAPOKE_LOCAL_TEST` nunca devem entrar no instalador público
  (`web/index.html`, `web/manifest.json`). O build público não contém comandos de depuração (`TESTMON`, `TESTEVO`, …).
- Não adicionar em massa binários históricos de firmware ao Git; apenas as quatro partes de uma versão liberada.
- Nenhum cry original de Pokémon, sample de ROM ou arquivo de áudio protegido por direitos autorais; o áudio permanece sintético.
- Os textos de UI no código do firmware permanecem majoritariamente em ASCII por causa da fonte (sem acentos/cedilhas).

## Build e testes

```bash
cd tests && make clean && make test
cd .. && bash tools/build_web.sh --check
bash tools/build_web_local.sh --check
bash tools/build_web_gen2_legacy.sh --check
git diff --check
```

Instalador local: `python3 -m http.server 8000 --directory web` (Chrome/Edge, `/`, `/dev.html`, `/release-gen3.html`).
Hardware (recorte do display, touch, IMU, volume, transferência via SD, preservação do save) é a última instância; testes nativos não substituem isso.
A unidade é ExFAT: antes de `make test`, na pasta `tests`, execute se necessário `dot_clean -m tests` (arquivos `._*`).

## Manutenção da documentação

- As regras comuns ficam aqui em `AGENTS.md`; `CLAUDE.md` as importa com `@AGENTS.md`.
- Ao mudar de versão, alinhar em conjunto `PROJEKT_KONTEXT.md`, `AI_HANDOFF.md`, `progress.md`, `web/README.md` e `README.md`.
- Idioma: **toda a documentação do projeto e as respostas da IA em português do Brasil (pt-BR)**, com acentuação correta.
  Exceções públicas em inglês, fora da ordem de leitura: `README.md` e `CREDITS.md`.
  Termos técnicos, nomes de arquivos, comandos e textos exibidos pelo firmware permanecem como estão.

<!-- skill-routing:start v2 – criado manualmente para o firmware ESP32; regras comuns em AGENTS.md; CLAUDE.md as importa -->
## Mapeamento de skills

Tipo de projeto: firmware ESP32 (Arduino/C++) com instalador web.

| Quando | Skill |
| --- | --- |
| Verificar o instalador web (`web/*.html`, `installer-gen3.js`) no navegador: cliques, erros no console, capturas de tela | `playwright` |
| Construir ou revisar páginas do instalador: estados, layout responsivo, acessibilidade, textos da interface | `ui-ux-quality` |
| Antes de publicar um novo estado do instalador | Seção "Build e testes" acima; publicar somente com solicitação explícita |
| Somente por pedido explícito: analisar ou recriar um app de terceiros | `replica-*` (início: `replica-recon`) |

Não se aplica: `apple-app-release-check`, `swiftui-expert-skill`, `swift-concurrency`, `fullstack-architecture`
(não é um app da App Store, não usa SwiftUI, não tem backend).
<!-- skill-routing:end -->
