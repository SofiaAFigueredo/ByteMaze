# ByteMaze

[![1.44MB Game Dev Contest](https://img.shields.io/badge/Contest-1.44MB%20Game%20Dev-blueviolet)](https://2pgarcade.com/contest-144mb.html)
[![Language](https://img.shields.io/badge/Language-C-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Engine](https://img.shields.io/badge/Engine-raylib-blue)](https://www.raylib.com/)
[![License](https://img.shields.io/badge/License-zlib%2Flibpng-green)](raylib/LICENSE)

**ByteMaze** e um jogo de acao e sobrevivencia em labirintos procedurais. O projeto foi feito em **C** com **raylib**, com foco em executavel independente e pacote final abaixo de 1.44 MB.

## Competicao

O jogo foi preparado para o **[1.44MB Game Dev Contest](https://2pgarcade.com/contest-144mb.html)**, da **2P Game Arcade**.

Regras relevantes verificadas na pagina oficial:

- Limite descompactado: **1,474,560 bytes**.
- Engine livre, desde que executavel, runtime e assets caibam no limite.
- O jogo deve rodar como executavel independente; jogos de navegador nao sao aceitos.
- Criterios: jogo completo, dentro do tamanho e divertido.

## Como Jogar

| Tecla | Acao |
| :--- | :--- |
| `W` `A` `S` `D` ou `Setas` | Mover |
| `SHIFT` | Dash com cargas limitadas por round |
| `ESPACO` | Atirar na direcao do triangulo |
| `R` | Recarregar o pente |
| `TAB` | Abrir/fechar mapa tatico |
| `C` | Ligar/desligar lanterna |
| `F` | Usar uma carga manual de raio |

## Definicoes de Gameplay

| Sistema | Valor |
| :--- | :--- |
| Vida inicial | `50` |
| Vida maxima | `100` |
| Loja de vida | `+5` de vida maxima por `100` moedas |
| Cura no chao | Restaura `15` de vida |
| Pente da arma | Maximo de `15` balas carregadas |
| Municao inicial | `30` municoes totais apenas no inicio da corrida |
| Municao maxima | `100` municoes totais |
| Loja de municao | `+30` municoes por `100` moedas, respeitando o limite de `100` |
| Municao no chao | `+10` municoes |
| Bateria da lanterna | Drena `1%` por segundo ligada; ligar custa `1%` |
| Loja da lanterna | Compra soma `+50%` de bateria por `100` moedas, ate `100%` |
| Bateria do mapa | Drena `1%` por segundo aberto; abrir custa `1%` |
| Loja do mapa | Compra soma `+50%` de bateria por `100` moedas, ate `100%` |
| Bateria no chao | Recarrega `+15%` na lanterna e no mapa |
| Moedas no chao | `+25` moedas |
| Recompensa por round | `+50` moedas |

## Inimigos e Progressao

- Inimigos vermelhos patrulham e perseguem quando o jogador esta perto.
- Inimigos rosas atiram quando ficam alinhados com o jogador em linha ou coluna sem parede.
- O chefao roxo persegue, atira quando alinhado e acelera quando esta longe.
- Todos os inimigos ficam nocauteados por `5` segundos apos `5` acertos.
- Rounds avancados adicionam chave/saida trancada, armadilhas, baixa visibilidade, mapa bloqueado, menos dash e maior pressao do chefao.
- Recursos coletados durante um round so ficam salvos se o round for vencido.
- A municao total e carregada entre rounds: o que sobra continua, e compras ou caixas no chao somam em cima desse saldo ate o limite de `100`.

## Tamanho

A regra da competicao considera o conteudo final descompactado. A medicao local soma o executavel e `src/assets`.

| Componente | Tamanho atual |
| :--- | ---: |
| `bytemaze` gerado por `make release` no Linux | `577,528` bytes |
| `src/assets` | `48,404` bytes |
| Total medido por `make size` | `625,932` bytes |
| Limite | `1,474,560` bytes |
| Uso | `42.45%` |
| Folga | `848,628` bytes |

O arquivo `bytemaze.exe` presente no diretorio tem `772,608` bytes. Somado aos assets atuais, fica em `821,012` bytes (`55.68%` do limite), ainda abaixo da regra de `1,474,560` bytes.

Use `make size` antes de enviar o pacote final.

## Compilar e Executar

Pre-requisitos:

- `gcc` ou `clang`
- `cmake`
- `make`
- raylib presente no diretorio `raylib/`

Comandos:

```sh
make release
make run
make size
```

No Linux o alvo gera `bytemaze`; no Windows o alvo gera `bytemaze.exe`.

## Arquivos Principais

```text
.
├── Makefile
├── README.md
├── bytemaze.exe
├── src/
│   ├── main.c
│   └── assets/
│       └── fonts/
└── raylib/
```

`bytemaze_save.dat` e criado apenas para progresso local do jogador e nao deve entrar na submissao oficial.

## Licenca

Este projeto usa raylib. Consulte [`raylib/LICENSE`](raylib/LICENSE).
