# 🇺🇸 English Version

# ByteMaze

> Unforgiving procedural maze survival packed into less than 1.44 MB.

**ByteMaze** is an action procedural maze survival game built in **C++** using **raylib**, focusing on infinite progression, high scores, and risk management under shrinking visibility.

The project was created specifically to compete in the **1.44MB Game Dev Contest**, pushing the boundaries of executable size optimization for a standalone application.

---

## Developer

* **GitHub:** [@SofiaAFigueredo](https://github.com/SofiaAFigueredo)

---

## 💾 About the Contest

The **1.44MB Game Dev Contest** challenges developers to build complete games that fit entirely within the memory footprint of a classic 3.5" floppy disk: **1,474,560 uncompressed bytes**.

* **Strict Limit:** $\le$ 1,474,560 bytes (uncompressed final executable).
* **Format:** Standalone executable (browser games are not allowed).
* **Scope:** Engine/runtime size counts toward the total byte limit.

---

## 🕹️ Concept & Gameplay

In **ByteMaze**, the player controls a geometric entity (Triangle) trapped inside a procedurally generated maze complex. The main objective is to reach the exit and maximize the score before eventually being caught.

### 🔄 Core Loop

```
┌───────────────────────────────────────────────────────────┐
│  1. Generate procedural maze                              │
│  2. Spawn player, exit, and hazards                       │
│  3. Navigate and fight enemies as visibility shrinks      │
│  4. Accumulate score (Level + Time + Kills)               │
│  5. Advance level ──► [Increase Difficulty]               │
└───────────────────────────────────────────────────────────┘

```

---

## ⚙️ Key Features

* **Procedural Generation:** Unique maze topology generated on the fly every level.
* **Shrinking Vision & Claustrophobia:**
* **Levels 1–4:** Full maze visibility.
* **Level 5+:** Field of view gradually decreases to a tight circular spotlight around the player.
* **Level 10+:** To assist against total darkness, the full map is briefly revealed for **5 seconds** at the start of each level.


* **Precision Combat:** Red laser projectiles to clear common threats.
* **Enemies & Hazards:**
* **Common Enemies (Orbs):** Three difficulty tiers visually indicated by distinct outlines.
* **Supreme Enemy (Square):** A persistent threat spawning far from the player that aggressively pursues you with increasing speed each level.



---

## 🎨 Visual & Audio Direction

To satisfy the strict *Byte Budget* without sacrificing style, game assets are generated at runtime along with minimal audio footprints:

| Element | Visual / Audio Representation |
| --- | --- |
| **Player** | High-contrast, bright geometric triangle |
| **Common Enemies** | Transparent circles with color-coded outline tiers |
| **Supreme Enemy** | High-priority pursuing square |
| **Maze** | Dark walls with light gray walkable paths |
| **Sound Effects** | Minimal/procedural audio for shots, enemy defeat, and victory |

---

## 🛠️ Engineering & Size Optimization (*Byte Budget*)

Technical feasibility under 1.44 MB is achieved through:

1. **Vector/Primitive Rendering:** Zero external textures; interface and gameplay actors are rendered using Raylib drawing primitives (`DrawTriangle`, `DrawCircleLines`, `DrawRectangle`).
2. **Procedural Audio / Heavy Compression:** Compact synthetic sound effects with minimal memory footprint.
3. **Optimized Build Flags:** Compilation using `-Os`, debug symbol stripping (`strip`), and Link Time Optimization (LTO).

---

## 📂 Documentation Structure

Detailed architecture and development logs are hosted in [`.codex/`](./.codex/README.md):

* [`01-game-vision.md`](./.codex/01-game-vision.md) — Game Design Document (GDD) and core vision.
* [`02-development-step-by-step.md`](./.codex/02-development-step-by-step.md) — Code architecture and step-by-step milestones.
* [`03-roadmap.md`](./.codex/03-roadmap.md) — Project roadmap and deliverables.
* [`04-byte-budget.md`](./.codex/04-byte-budget.md) — Size allocation and optimization planning.
* [`05-session-log.md`](./.codex/05-session-log.md) — Work log and technical decisions.

---

## 📌 Project Status

> **Current Phase:** 🟡 Technical Definition & Planning
---

# 🇧🇷 Português 

# ByteMaze

> Um jogo de sobrevivência em labirintos procedurais dentro do limite de 1,44 MB.

**ByteMaze** é um jogo de ação e sobrevivência em labirinto procedural, desenvolvido em **C++** com **raylib**, focado em progressão infinita, pontuação alta e gestão de risco sob visibilidade reduzida.

O projeto foi concebido especificamente para participar do **1.44MB Game Dev Contest**, desafiando os limites de otimização de tamanho de executável standalone.

---

## Desenvolvedora

* **GitHub:** [@SofiaAFigueredo](https://github.com/SofiaAFigueredo)

---

## 💾 Sobre o Concurso

O **1.44MB Game Dev Contest** desafia desenvolvedores a criarem jogos completos que caibam inteiramente no espaço equivalente a um antigo disquete de 3,5": **1.474.560 bytes descompactados**.

* **Limite Rígido:** $\le$ 1.474.560 bytes (executável final descompactado).
* **Formato:** Executável standalone (jogos de navegador não são permitidos).
* **Escopo:** O tamanho da engine/runtime conta no limite total.

---

## 🕹️ Conceito & Gameplay

Em **ByteMaze**, o jogador controla uma entidade geométrica (Triângulo) presa em um complexo de labirintos gerados proceduralmente. O objetivo é alcançar a saída e pontuar o máximo possível antes de ser inevitavelmente alcançado.

### 🔄 Loop Principal

```
┌───────────────────────────────────────────────────────────┐
│  1. Gerar labirinto procedural                            │
│  2. Posicionar jogador, saída e perigos                   │
│  3. Navegar e combater inimigos enquanto a visão reduz    │
│  4. Acumular pontuação (Nível + Tempo + Eliminações)      │
│  5. Avançar de nível ──► [Aumentar Dificuldade]           │
└───────────────────────────────────────────────────────────┘

```

---

## ⚙️ Mecânicas Principais

* **Geração Procedural:** Cada nível possui uma topologia única gerada via código.
* **Visão Cega & Claustrofobia:**
* **Nível 1–4:** Visão completa do mapa.
* **Nível 5+:** A visão diminui gradualmente até se tornar um pequeno holofote circular ao redor do jogador.
* **Nível 10+:** Como auxílio contra a escuridão, o mapa completo é revelado por **5 segundos** no início do nível.


* **Combate Preciso:** Disparos de projéteis vermelhos para eliminar perigos comuns.
* **Inimigos & Ameaças:**
* **Inimigos Comuns (Esferas):** Três níveis de dificuldade identificados pelas orlas.
* **Inimigo Supremo (Quadrado):** Uma ameaça implacável que surge longe do jogador e o persegue com agressividade crescente a cada nível.



---

## 🎨 Direção Visual & Sonora

Para garantir um *Byte Budget* estrito sem comprometer a estética, o jogo utiliza arte gerada em tempo de execução e áudio sintetizado/enxuto:

| Elemento | Representação Visual / Sonora |
| --- | --- |
| **Jogador** | Triângulo de cor chamativa em alto contraste |
| **Inimigos Comuns** | Círculos transparentes com bordas indicativas de nível |
| **Inimigo Supremo** | Quadrado perseguidor de cor destacada |
| **Labirinto** | Paredes escuras e corredores em cinza claro |
| **Efeitos Sonoros** | Sfx procedural/enxuto para tiro, destruição e vitória |

---

## 🛠️ Engenharia & Otimização de Tamanho (*Byte Budget*)

A viabilidade técnica em < 1,44 MB é mantida através de:

1. **Rendering Vetorial/Primitivas:** Sem texturas externas; toda a interface e atores são desenhados usando primitivas gráficas da Raylib (`DrawTriangle`, `DrawCircleLines`, `DrawRectangle`).
2. **Audio Procedural/Compressão Agressiva:** Efeitos de áudio sintéticos com consumo mínimo de memória.
3. **Flags de Compilação Otimizadas:** Uso de `-Os`, remoção de símbolos de debug (`strip`), e LTO (*Link Time Optimization*).

---

## 📂 Estrutura de Documentação

Os documentos detalhados de arquitetura e progresso estão organizados em [`.codex/`](./.codex/README.md):

* [`01-game-vision.md`](./.codex/01-game-vision.md) — Visão detalhada do jogo e Game Design Document (GDD).
* [`02-development-step-by-step.md`](./.codex/02-development-step-by-step.md) — Etapas de desenvolvimento e arquitetura de código.
* [`03-roadmap.md`](./.codex/03-roadmap.md) — Cronograma e entregáveis.
* [`04-byte-budget.md`](./.codex/04-byte-budget.md) — Planejamento e controle do limite de 1,44 MB.
* [`05-session-log.md`](./.codex/05-session-log.md) — Diário de bordo e decisões técnicas.

---

## 📌 Status do Projeto

> **Fase Atual:** 🟡 Definição & Planejamento Técnico