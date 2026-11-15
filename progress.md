# Progresso do desenvolvimento do TamaPoke

Situação em: 2026-11-05

## Estado atual

- Branch ativo: `local/full-gen3`; tag de rollback: `local-before-gen3`.
- O commit de funcionalidade Gen 3 testado, `da8bfa8`, já está em `fork/main`.
- O instalador hospedado entrega a Gen 3 pública `1.36.2` com #1–386.
- O GitHub Pages foi trocado para o instalador Gen 3 e o release do GitHub correspondente foi publicado.
- Build de depuração local: `1.36.2-hoenn-walk-local` com `TAMAPOKE_LOCAL_TEST`
  (expedições a pé em Hoenn; testado no hardware pelo responsável há várias semanas; ver
  `HOENN_WALK_LOCAL.md`).
- Release público: `1.36.2` com `TAMAPOKE_GEN3_RELEASE`, sem comandos de depuração locais.

## Funcionalidades concluídas

- Dados do Dex Gen 2 e Gen 3 até #386, seis idiomas, atributos, tipos, dados de raridade,
  seleção de região/starter e regras de evolução.
- A evolução exige estritamente três de quatro valores de cuidado acima de 40 e suporta
  condições de ramificação, vínculo, dia/noite e atributos.
- Todos os arquivos SpriteCollab normais e Shiny até #386, mais 386 miniaturas.
- Marca d'água `@SE` de atribuição em movimento.
- Contador de passos persistente por software, HUD no canto superior esquerdo e cartão Passos/Trilha.
- Recompensas diárias em 500/2.000/5.000 passos, classificações vitalícias de trilha,
  recompensas de JOY/VÍNCULO por caminhada e bônus limitados de Shiny/captura.
- Controles locais no navegador para Pokémon, evoluções, batalhas e diagnósticos de IMU/passos;
  nenhum terminal é necessário para o fluxo de depuração.
- Reforço do build Gen 3: configuração de evolução por alvo, tratamento de timeout e reconexão do
  Web Serial, reaproveitamento da lista da Caixa e validação completa das miniaturas.

## Entrega de sprites

- `web/sprites.pak`: 503 entradas para #1–251, incluindo um arquivo de miniaturas com 251 entradas.
- `web/sprites-gen3-update.pak`: 271 entradas para #252–386, incluindo o arquivo completo de miniaturas com 386 entradas.
- Uma instalação Gen 3 completa envia esses dois pacotes em sequência.
- `sprites-gen3-full.pak` foi removido porque excedia o limite de 100 MB do GitHub
  e não pode ser referenciado nas instruções atuais.

## Caminhos do instalador

- `/web/index.html`: instalador Gen 3 público, ativo.
- `/web/dev.html`: instalador Gen 3 de depuração local, com opções opcionais de atualização/completo.
- `/web/release-gen3.html`: cópia local de referência com um único botão obrigatório de sprites completos
  e sem controles de depuração.

## Marcos históricos

- `local-before-full-gen2`: rollback antes do trabalho completo da Gen 2.
- `local-before-steps`: rollback antes do sistema de caminhada.
- `local-full-gen2-final`: snapshot concluído do desenvolvimento da Gen 2.
- `local-full-gen2-final-1.35.3` / `v1.35.3`: instalador público estável da Gen 2.
- `local-before-gen3-bugfix-20260825`: snapshot antes do reforço da Gen 3.

## Próximos passos

- Acompanhar instalações reais e manter em destaque o aviso de sprites em duas etapas.
- Continuar testes longos de hardware (soak tests) para caminhada com tela desligada, recompensas,
  seis idiomas, bordas do touch e preservação do save.
