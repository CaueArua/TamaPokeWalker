# Candidato de expedição a pé em Hoenn

Firmware local: `1.36.2-hoenn-walk-local`. Instalador público: `1.36.2` (mesmas funções, como build de release sem depuração).
O build de depuração fica local; o release público é publicado somente sob solicitação.

## Bônus de caminhada

Escolha um local e uma duração. As metas são 250 / 500 / 1.000 passos para
15 / 30 / 60 minutos. Somente os passos detectados durante a expedição contam,
inclusive com o display desligado e atravessando a meia-noite. Não é preciso
caminhar para obter o item normal nem o selo do local.

O item adicional é energia na costa, cuidado na floresta e treino no vulcão.
Os bônus de costa/floresta têm 10 / 15 / 20% de chance de melhoria de treino,
fixada na partida. Se o slot de bônus estiver cheio ou o treino já estiver no
máximo, o bônus dá 10 de energia. Ao reivindicar a expedição, o bônus é pago uma única vez.

O backup de save versão 2 inclui o progresso de caminhada. Backups públicos da 1.36.1
e backups locais anteriores de Hoenn são aceitos; expedições existentes mantêm
a recompensa principal, sem bônus de caminhada retroativo.

## Aceitação em hardware local (marcada como concluída)

O responsável relatou em 06/11/2026 que este build roda no hardware há várias semanas.
A checklist abaixo é mantida como procedimento de referência.

Execute `bash tools/build_web_local.sh`, sirva `web` em localhost e grave usando
`/dev.html`. Deixe "Erase device" desmarcado. Os sprites não precisam ser reinstalados.

- Verificar os nomes dos locais, as metas e o texto do bônus obtido nos seis idiomas.
- Iniciar uma expedição, caminhar com o display ligado/desligado e conferir o progresso.
- Reiniciar durante uma expedição e confirmar que o progresso salvo permanece.
- Reivindicar um item normal e um bônus de caminhada, depois confirmar que não há segunda reivindicação.
- Conferir que, com o inventário de bônus cheio ou o treino no máximo, o resultado é energia.
- Conferir que backup/restauração preserva a expedição ativa e o bônus obtido.

Estes testes físicos não podem ser substituídos pela suíte de testes nativa.
