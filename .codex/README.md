# .codex

Esta pasta guarda o contexto vivo do projeto para que o jogo evolua com consistencia.

## Objetivo

Manter em um unico lugar:

- a visao do jogo;
- as regras de design;
- a ordem de implementacao;
- o controle de escopo;
- o cuidado com o limite de `1,44 MB`;
- o historico de decisoes e proximos passos.

## Ordem de leitura

Antes de qualquer nova rodada de desenvolvimento, leia nesta ordem:

1. [01-game-vision.md](/home/davi/Documentos/GitHub/a-mudar/.codex/01-game-vision.md)
2. [02-development-step-by-step.md](/home/davi/Documentos/GitHub/a-mudar/.codex/02-development-step-by-step.md)
3. [03-roadmap.md](/home/davi/Documentos/GitHub/a-mudar/.codex/03-roadmap.md)
4. [04-byte-budget.md](/home/davi/Documentos/GitHub/a-mudar/.codex/04-byte-budget.md)
5. [05-session-log.md](/home/davi/Documentos/GitHub/a-mudar/.codex/05-session-log.md)

## Como usar esta pasta

- `01-game-vision.md`: define o jogo e protege a ideia principal.
- `02-development-step-by-step.md`: explica exatamente o que implementar e em qual ordem.
- `03-roadmap.md`: mostra as fases do projeto e checklists.
- `04-byte-budget.md`: ajuda a nao estourar o limite do concurso.
- `05-session-log.md`: registra decisoes, progresso e bloqueios.

## Regra de evolucao

Sempre que uma decisao importante for tomada, atualize:

- `01-game-vision.md` se a ideia central mudar;
- `03-roadmap.md` se o status das fases mudar;
- `04-byte-budget.md` se entrar ou sair algum asset;
- `05-session-log.md` ao final de cada sessao relevante.

## Norte do projeto

Este jogo precisa ser:

- simples de entender;
- rapido de jogar;
- pequeno em tamanho;
- crescente em tensao;
- facil de expandir por niveis;
- forte o suficiente para competir mesmo com recursos visuais minimos.
