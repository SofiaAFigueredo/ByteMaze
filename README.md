# 🌀 ByteMaze


**An action-survival game set in procedural mazes, focused on precision, strategy, and extreme size optimization.**
> **Um jogo de ação e sobrevivência em labirintos procedurais com foco em precisão, estratégia e otimização extrema.**


[![1.44MB Game Dev Contest](https://img.shields.io/badge/Contest-1.44MB%20Game%20Dev-8A2BE2?style=for-the-badge&logo=itchio&logoColor=white)](https://2pgarcade.com/contest-144mb.html)
[![Language](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Engine](https://img.shields.io/badge/raylib-000000?style=for-the-badge&logo=raylib&logoColor=white)](https://www.raylib.com/)
[![License](https://img.shields.io/badge/License-zlib%2Flibpng-green?style=for-the-badge)](raylib/LICENSE)

---

## 📄 Table of Contents / Sumário

- [English](#-english)
  - [About the Project](#-about-the-project)
  - [Controls](#-controls)
  - [Systems & Gameplay Definitions](#️-systems--gameplay-definitions)
  - [Enemies & Hazards](#-enemies--hazards)
  - [Size Budget](#-size-budget)
  - [Build & Run](#️-build--run)
  - [Repository Structure](#-repository-structure)
  - [Developer & Credits](#-developer--credits)
- [Português (Brasil)](#-português-brasil)
  - [Sobre o Projeto](#-sobre-o-projeto)
  - [Controles](#-controles)
  - [Sistemas & Definições de Gameplay](#️-sistemas--definições-de-gameplay)
  - [Inimigos & Perigos](#-inimigos--perigos)
  - [Orçamento de Tamanho](#-orçamento-de-tamanho)
  - [Compilação & Execução](#️-compilação--execução)
  - [Estrutura do Repositório](#-estrutura-do-repositório)
  - [Desenvolvedora & Créditos](#-desenvolvedora--créditos)

---

## 🇺🇸 English


## 💡 About the Project

**ByteMaze** combines fast survival action with tactical exploration in procedurally generated mazes. Built from scratch in **C** using **raylib**, the game was designed to deliver a modern experience while keeping the final size extremely small.

> 🏆 **1.44MB Game Dev Contest (2P Game Arcade)**\
> Created for the challenge of making a complete, offline, standalone, and fun game that fits within the classic 3.5" floppy disk size limit (**1,474,560 bytes**).

---

## 🎮 Controls

| Key | Action |
| :---: | :--- |
| `W` `A` `S` `D` / `Arrow Keys` | Move the player |
| `SHIFT` | **Dash**: quick burst movement with limited charges per round |
| `SPACE` | **Shoot**: fires toward the aiming direction |
| `R` | **Reload**: reloads the weapon magazine |
| `TAB` | **Tactical Map**: opens/closes the area view |
| `C` | **Flashlight**: toggles the vision light |
| `F` | **Lightning**: reveals the full maze for a few seconds |

---

## ⚙️ Systems & Gameplay Definitions

### 🔴 Health & Survival
- **Starting Health:** `50 HP` (maximum: `100 HP`)
- **Floor Health Pickup:** instantly restores `+15 HP`.
- **Health Shop:** increases maximum health by `+5 HP` for `100` coins.

### 🔫 Ammo & Combat
- **Weapon Magazine:** holds up to `15` loaded bullets.
- **Starting Ammo:** `30` total ammo only at the start of a run.
- **Maximum Ammo:** `100` total ammo.
- **Floor Ammo Pickup:** adds `+10` bullets.
- **Ammo Shop:** adds `+30` bullets for `100` coins.
- **Persistence:** leftover ammo carries between rounds; purchases and pickups add to the current total up to the `100` ammo limit.

### ⚡ Batteries (Flashlight & Map)
- **Active Drain:** drains `1%` per second of use + `1%` activation cost.
- **Floor Battery Pickup:** restores `+15%` to both tools.
- **Battery Shop:** restores `+50%` to both flashlight and map batteries for `100` coins, capped at `100%`.

### 🪙 Economy & Rewards
- **Floor Coins:** `+25` coins when collected.
- **Round Bonus:** `+50` coins for each victory.
- ⚠️ *Resources collected during a round are saved only if you win that round.*

---

## 👾 Enemies & Hazards

* **🔴 Red Enemies:** patrol the maze and chase the player when detected.
* **🩷 Pink Enemies:** attack from range when they have a clear horizontal or vertical line of sight.
* **🟣 Boss:** relentlessly chases the player, shoots when aligned, and accelerates when far away.
* **💫 Knockout:** hit any enemy **5 times** to temporarily disable it for **5 seconds**.
* **⚡ Advanced Levels:** require finding the golden key to unlock the exit, and introduce traps, low visibility, and map/radar blocking.

---

## 📊 Size Budget

The contest requires the executable and all assets to fit inside the uncompressed 1.44 MB envelope.

```text
📊 Project Occupancy Report:

┌────────────────────────┬─────────────────┬──────────┐
│ Component              │ Size (Bytes)    │ % of Max │
├────────────────────────┼─────────────────┼──────────┤
│ Linux Binary           │ ~577,528 B      │   39.16% │
│ Assets (Fonts)         │  ~48,404 B      │    3.28% │
├────────────────────────┼─────────────────┼──────────┤
│ MEASURED TOTAL         │  625,932 B      │   42.45% │
└────────────────────────┴─────────────────┴──────────┘

💾 Maximum Limit: 1,474,560 bytes
🟢 Free Space:      848,628 bytes (57.55%)
```

> ℹ️ The `ByteMaze-1.44MB-linux.zip` package contains the Linux binary, required assets, and this README. Uncompressed, it remains below the contest limit.

---

## 🛠️ Build & Run

### Requirements
Make sure the following tools are installed:
- C compiler (`gcc` or `clang`)
- `cmake`
- `make`
- Updated **raylib** submodule

### Build Steps

1. Clone the repository with dependencies:
```sh
git clone --recursive https://github.com/SofiaAFigueredo/ByteMaze.git
cd ByteMaze
```

2. Build the optimized release version:
```sh
make release
```

3. Run the game:
```sh
make run
```

4. Check size compliance for submission:
```sh
make size
```

---

## 📂 Repository Structure

```text
ByteMaze/
├── 📄 Makefile            # Automation and byte-check script
├── 📄 README.md           # Project documentation
├── 📁 src/
│   ├── 📄 main.c          # Complete game logic
│   └── 📁 assets/         # Essential resources
└── 📁 raylib/             # Graphics library source code
```

> 📌 *The local save file (`bytemaze_save.dat`) is ignored and is not part of the submission package.*

---

## 👤 Developer & Credits

- **Development:** Sofia A. Figueredo ([@SofiaAFigueredo](https://github.com/SofiaAFigueredo))
- **Technical assistance:** OpenAI Codex, used for code review support, balancing, build fixes, and documentation.
- **Graphics Engine:** This project uses [raylib](https://www.raylib.com/), distributed under the zlib/libpng license. See [`raylib/LICENSE`](raylib/LICENSE) for details.

---

## 🇧🇷 Português (Brasil)

## 💡 Sobre o Projeto

**ByteMaze** combina ação frenética de sobrevivência com exploração tática de labirintos gerados proceduralmente. Desenvolvido do zero na linguagem **C** utilizando a biblioteca **raylib**, o jogo foi concebido para entregar uma experiência moderna mantendo um consumo de espaço ultra-eficiente.

> 🏆 **1.44MB Game Dev Contest (2P Game Arcade)**\
> Desenvolvido especialmente para o desafio de criar um jogo completo, offline, independente e divertido que caiba no espaço histórico de um disquete de 3.5" (**1.474.560 bytes**).

---

## 🎮 Controles

| Tecla | Ação |
| :---: | :--- |
| `W` `A` `S` `D` / `Setas` | Movimentação do personagem |
| `SHIFT` | **Dash**: impulso rápido com cargas limitadas por round |
| `ESPAÇO` | **Atirar**: dispara na direção da mira |
| `R` | **Recarregar**: carrega o pente de munição |
| `TAB` | **Mapa Tático**: abre/fecha a visualização da área |
| `C` | **Lanterna**: alterna a iluminação da visão |
| `F` | **Raio**: revela o labirinto inteiro por alguns segundos |

---

## ⚙️ Sistemas & Definições de Gameplay

### 🔴 Vida & Sobrevivência
- **Vida Inicial:** `50 HP` (máximo: `100 HP`)
- **Cura no Chão:** restaura `+15 HP` instantaneamente.
- **Loja de Vida:** aumenta o limite máximo em `+5 HP` por `100` moedas.

### 🔫 Munição & Combate
- **Pente da Arma:** capacidade de `15` balas engatilhadas.
- **Munição Inicial:** `30` munições totais apenas no início da corrida.
- **Limite Máximo:** `100` munições totais.
- **Munição no Chão:** adiciona `+10` projéteis.
- **Loja de Munição:** adiciona `+30` projéteis por `100` moedas.
- **Persistência:** o que sobra continua entre rounds; compras e coletas somam em cima do saldo atual até o limite de `100`.

### ⚡ Baterias (Lanterna & Mapa)
- **Consumo Ativo:** drena `1%` por segundo de uso + custo de `1%` ao ativar.
- **Carga no Chão:** recarrega `+15%` em ambos os dispositivos.
- **Loja de Bateria:** recarrega `+50%` na lanterna e no mapa por `100` moedas, respeitando o limite de `100%`.

### 🪙 Economia & Recompensas
- **Moedas no Chão:** `+25` moedas ao coletar.
- **Bônus de Round:** `+50` moedas por vitória.
- ⚠️ *Atenção: os recursos coletados no round só são salvos se você vencer a etapa!*

---

## 👾 Inimigos & Perigos

* **🔴 Inimigos Vermelhos:** realizam patrulhas e perseguem o jogador quando detectado.
* **🩷 Inimigos Rosas:** atacam à distância assim que alinham visão direta no eixo horizontal ou vertical.
* **🟣 O Chefão (Boss):** persegue implacavelmente, dispara quando alinhado e ganha aceleração ao se distanciar.
* **💫 Nocaute:** acerte qualquer inimigo **5 vezes** para desativá-lo temporariamente por **5 segundos**.
* **⚡ Níveis Avançados:** exigem encontrar a chave dourada para desbloquear a saída, além de introduzir armadilhas, baixa visibilidade e bloqueio de radar.

---

## 📊 Orçamento de Tamanho

O limite rigoroso da competição exige que o executável e todos os assets caibam no envelope de 1.44 MB descompactado.

```text
📊 Relatório de Ocupação do Projeto:

┌────────────────────────┬─────────────────┬──────────┐
│ Componente             │ Tamanho (Bytes) │ % do Max │
├────────────────────────┼─────────────────┼──────────┤
│ Binário Linux          │ ~577,528 B      │   39.16% │
│ Assets (Fontes)        │  ~48,404 B      │    3.28% │
├────────────────────────┼─────────────────┼──────────┤
│ TOTAL MEDIDO           │  625,932 B      │   42.45% │
└────────────────────────┴─────────────────┴──────────┘

💾 Limite Máximo: 1,474,560 bytes
🟢 Margem Livre:   848,628 bytes (57.55%)
```

> ℹ️ O pacote `ByteMaze-1.44MB-linux.zip` contém o binário Linux, os assets necessários e este README. Descompactado, ele permanece abaixo do limite da competição.

---

## 🛠️ Compilação & Execução

### Pré-requisitos
Certifique-se de ter os seguintes utilitários instalados:
- Compilador C (`gcc` ou `clang`)
- `cmake`
- `make`
- Submódulo da biblioteca **raylib** atualizado

### Passos para Compilar

1. Clone o repositório com as dependências:
```sh
git clone --recursive https://github.com/SofiaAFigueredo/ByteMaze.git
cd ByteMaze
```

2. Compile a versão otimizada de lançamento:
```sh
make release
```

3. Execute o jogo:
```sh
make run
```

4. Verifique a conformidade de tamanho para a submissão:
```sh
make size
```

---

## 📂 Estrutura do Repositório

```text
ByteMaze/
├── 📄 Makefile            # Script de automação e checagem de bytes
├── 📄 README.md           # Documentação do projeto
├── 📁 src/
│   ├── 📄 main.c          # Lógica completa do jogo
│   └── 📁 assets/         # Recursos essenciais
└── 📁 raylib/             # Código-fonte da biblioteca gráfica
```

> 📌 *Nota: o arquivo de save gerado localmente (`bytemaze_save.dat`) é ignorado e não faz parte do pacote de submissão.*

---

## 👤 Desenvolvedora & Créditos

- **Desenvolvimento:** Sofia A. Figueredo ([@SofiaAFigueredo](https://github.com/SofiaAFigueredo))
- **Assistência técnica:** OpenAI Codex, usado como apoio em revisão de código, balanceamento, correções de build e documentação.
- **Engine Gráfica:** Este projeto utiliza a biblioteca [raylib](https://www.raylib.com/), distribuída sob a licença zlib/libpng. Consulte o arquivo [`raylib/LICENSE`](raylib/LICENSE) para mais detalhes.
