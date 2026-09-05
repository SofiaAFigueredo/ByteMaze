
# ByteMaze

[![1.44MB Game Dev Contest](https://img.shields.io/badge/Contest-1.44MB%20Game%20Dev-blueviolet)](https://2pgarcade.com/contest-144mb.html)
[![Language](https://img.shields.io/badge/Language-C-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Library](https://img.shields.io/badge/Engine-raylib-raylib)](https://www.raylib.com/)
[![License](https://img.shields.io/badge/License-zlib%2Flibpng-green)](raylib/LICENSE)

**ByteMaze** é um jogo de ação e sobrevivência rápida ambientado em labirintos procedurais. Desenvolvido em linguagem **C** utilizando a biblioteca **raylib**, o projeto foi projetado do zero sob regras rigorosas de otimização de tamanho.

---

## 🏆 Sobre a Competição

O jogo foi desenvolvido para o **[1.44MB Game Dev Contest](https://2pgarcade.com/contest-144mb.html)**, organizado pela **2P Game Arcade**. O grande desafio da competição é fazer um jogo moderno, completo e divertido caber no espaço histórico de um disquete de 3.5" (1.44 MB).

### Directrizes e Restrições Atendidas:
* **Tamanho Máximo Total:** $\le 1.474.560 \text{ bytes}$ após descompactar o pacote final.
* **Stand-alone / Offline:** Executável independente que roda localmente sem requisições web ou servidores externos.
* **Engine & Assets Integrados:** O tamanho final contabiliza o binário gerado, a runtime e todos os assets gráficos e de fonte inclusos.

---

## 🕹️ Como Jogar

### Controles Gerais

| Tecla | Ação |
| :--- | :--- |
| `W` `A` `S` `D` ou `Setas` | Movimentação do personagem |
| `SHIFT` | **Dash**: Impulso rápido na direção atual (cargas limitadas) |
| `ESPAÇO` | **Atirar**: Dispara na direção do ponteiro/triângulo |
| `R` | **Recarregar**: Carrega o pente de munição |
| `TAB` | **Mapa Tático**: Revela o mapa (consome bateria dedicada) |
| `C` | **Lanterna**: Liga/desliga o iluminador (consome bateria) |
| `F` | **Raio**: Ativa uma carga manual de choque nos inimigos |

---

## ⚙️ Mecanicas do Jogo

- **Labirintos Procedurais:** Cada round gera uma estrutura única de caminhos e obstáculos.
- **Condição de Vitória:** Alcance e pise no **bloco verde** para avançar para o próximo nível.
- **Saída Trancada:** Em níveis avançados, a saída verde só é liberada após encontrar a **chave dourada**.
- **Gestão de Recursos:**
  - **Munição:** Caixas pelo mapa adicionam 10 balas (limite máximo acumulável: 100).
  - **Baterias:** O mapa tático (`TAB`) e a lanterna (`C`) utilizam cargas de bateria independentes.
- **Combate e Inimigos:**
  - **Inimigos Comuns:** Podem ser temporariamente nocauteados por alguns segundos ao receberem tiros suficientes.
  - **O Chefão (Boss):** Persegue ativamente o jogador, acelera quando a distância aumenta e dispara projéteis assim que obtém linha de visão direta.
- **Dificuldade Progressiva:** Rounds mais avançados introduzem armadilhas, perda de visibilidade, bloqueio no mapa, restrição de *dashes* e maior agressividade do chefão.

---

## 📊 Estatísticas e Orçamento de Tamanho

A otimização de espaço foi fundamental no desenvolvimento do ByteMaze. A verificação do tamanho considera o executável compilado junto da pasta de assets obrigatória (`src/assets`).

| Componente | Tamanho (Bytes) | % do Limite (1.44 MB) |
| :--- | :--- | :--- |
| **Executável Linux** | ~577.528 B | 39.16% |
| **Assets (`src/assets`)** | ~48.404 B | 3.28% |
| **Total Atual** | **625.932 B** | **42.45%** |

> ⚠️ **Nota para Build Windows:** Ao compilar via MinGW/GCC para Windows (`bytemaze.exe`), certifique-se de executar `make size` para garantir que a soma das dependências permaneça abaixo dos 1.474.560 bytes exigidos.

---

## 🛠️ Como Compilar e Executar

### Pré-requisitos
- Compilador C (`gcc` ou `clang`).
- **CMake** e **Make**.
- Submódulo da biblioteca **raylib** clonado dentro do diretório.

### Comandos de Build

1. Clone o repositório com os submódulos:
   ```sh
   git clone --recursive [https://github.com/SofiaAFigueredo/ByteMaze.git](https://github.com/SofiaAFigueredo/ByteMaze.git)
   cd ByteMaze

```

2. Gere a versão de release otimizada:
```sh
make release

```


3. Verifique se o pacote cumpre as regras de tamanho da competição:
```sh
make size

```



---

## 📂 Estrutura de Arquivos Principais

```text
├── Makefile                   # Script de build otimizado e medição de tamanho
├── src/
│   ├── main.c                 # Lógica e código completo do jogo
│   └── assets/
│       └── fonts/             # Subset de fonte otimizado (NotoSansKR)
└── raylib/                    # Submódulo do Raylib

```

> 📌 **Aviso:** O arquivo `bytemaze_save.dat` é utilizado apenas para o progresso local do jogador e **não** deve ser incluído na submissão oficial para a 2P Game Arcade.

---

## 👤 Desenvolvedora

* **Sofia A. Figueredo**
* GitHub: [@SofiaAFigueredo](https://github.com/SofiaAFigueredo)

*O desenvolvimento contou com o suporte do OpenAI Codex para otimização técnica, refatoração de código, balanceamento de jogabilidade e documentação.*

---

## 📜 Licença

Este projeto faz uso da biblioteca de código aberto **raylib**. Consulte os termos de uso em [`raylib/LICENSE`](https://www.google.com/search?q=raylib/LICENSE).