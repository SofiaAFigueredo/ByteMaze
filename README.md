# ByteMaze

ByteMaze e um jogo de acao e sobrevivencia em labirintos procedurais, feito em C com raylib para o 1.44MB Game Dev Contest.

O objetivo e atravessar cada labirinto, encontrar a chave quando a saida estiver trancada e pisar no bloco verde para avancar. A cada round o jogo mistura inimigos, visibilidade, armadilhas, recursos e modificadores para manter a partida evoluindo dentro do limite de 1,474,560 bytes descompactados.

## Desenvolvedora

Sofia A. Figueredo  
GitHub: [@SofiaAFigueredo](https://github.com/SofiaAFigueredo)

O desenvolvimento foi auxiliado pelo Codex para revisao de codigo, ajustes de jogabilidade, organizacao tecnica e documentacao.

## Competicao

ByteMaze foi preparado para o 1.44MB Game Dev Contest da 2P Game Arcade.

Regras principais consideradas:

- Limite total apos descompactar: 1,474,560 bytes.
- O jogo precisa rodar como executavel independente.
- Jogos de navegador nao sao aceitos.
- Engine e runtime contam dentro do limite.

## Como Jogar

- `WASD` ou setas: mover.
- `SHIFT`: dash na direcao atual, com cargas limitadas por round.
- `ESPACO`: atirar na direcao do triangulo.
- `R`: recarregar.
- `TAB`: abrir o mapa tatico enquanto houver bateria do mapa.
- `C`: ligar/desligar a lanterna quando ela estiver disponivel.
- `F`: usar uma carga manual de raio quando disponivel.

## Mecanicas

- Labirinto procedural novo a cada round.
- Saida verde obrigatoria para concluir o round.
- Rounds com saida trancada exigem encontrar a chave dourada.
- O mapa e a lanterna possuem baterias separadas.
- Caixas de municao adicionam 10 balas.
- A municao total e limitada a 100.
- Inimigos podem ser nocauteados por alguns segundos apos receberem tiros suficientes.
- O chefao persegue o jogador, atira quando fica alinhado e acelera quando esta longe.
- Rounds avancados adicionam armadilhas, baixa visibilidade, mapa bloqueado, menos dash e pressao do chefao.

## Build

Requisitos:

- Compilador C.
- CMake.
- raylib incluida como submodulo/pasta do projeto.

Comandos:

```sh
make release
make size
```

O alvo `make size` soma o executavel gerado e os assets em `src/assets`, que e o conjunto necessario para rodar o jogo.

## Tamanho Atual

Ultima verificacao local:

```text
625932 bytes (577528 executavel + 48404 assets, 42.45% de 1474560)
```

Esse resultado e para o binario Linux `bytemaze` gerado neste ambiente. Para enviar Windows, recompile `bytemaze.exe` em um ambiente Windows/MinGW e confira novamente que o executavel mais `src/assets` continuam abaixo de 1,474,560 bytes.

## Arquivos Relevantes

- `src/main.c`: codigo completo do jogo.
- `src/assets/fonts/NotoSansKR-Subset-Bold.ttf`: fonte incluida no pacote final.
- `Makefile`: build otimizado e verificacao de tamanho.

Nao envie `bytemaze_save.dat`; ele e apenas progresso local do jogador.

## Licenca

Este repositorio usa raylib como biblioteca grafica. Consulte a licenca da raylib em `raylib/LICENSE`.
