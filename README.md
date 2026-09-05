# 🌀 ByteMaze

**Um jogo de ação e sobrevivência em labirintos procedurais com foco em precisão, estratégia e otimização extrema.**

[![1.44MB Game Dev Contest](https://img.shields.io/badge/Contest-1.44MB%20Game%20Dev-8A2BE2?style=for-the-badge&logo=itchio&logoColor=white)](https://2pgarcade.com/contest-144mb.html)
[![Language](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Engine](https://img.shields.io/badge/raylib-000000?style=for-the-badge&logo=raylib&logoColor=white)](https://www.raylib.com/)
[![License](https://img.shields.io/badge/License-zlib%2Flibpng-green?style=for-the-badge)](raylib/LICENSE)

---

## 💡 Sobre o Projeto

**ByteMaze** combina ação frenética de sobrevivência com exploração tática de labirintos gerados proceduralmente. Desenvolvido do zero na linguagem **C** utilizando a biblioteca **raylib**, o jogo foi concebido para entregar uma experiência moderna mantendo um consumo de espaço ultra-eficiente.

> 🏆 **1.44MB Game Dev Contest (2P Game Arcade)**\
> Desenvolvido especialmente para o desafio de criar um jogo completo, offline e divertido que caiba no espaço histórico de um disquete de 3.5" (**1.474.560 bytes**).

---

## 🎮 Controles

| Tecla | Ação |
| :---: | :--- |
| `W` `A` `S` `D` / `Setas` | Movimentação do personagem |
| `SHIFT` | **Dash**: Impulso rápido (cargas limitadas por round) |
| `ESPAÇO` | **Atirar**: Dispara na direção da mira |
| `R` | **Recarregar**: Carrega o pente de munição |
| `TAB` | **Mapa Tático**: Abre/fecha a visualização da área |
| `C` | **Lanterna**: Alterna a iluminação da visão |
| `F` | **Raio**: Revela o labirinto inteiro por alguns segundos |

---

## ⚙️ Sistemas & Definições de Gameplay

### 🔴 Vida & Sobrevivência
- **Vida Inicial:** `50 HP` (máximo: `100 HP`)
- **Cura no Chão:** Restaura `+15 HP` instantaneamente.
- **Loja de Vida:** Aumenta o limite máximo em `+5 HP` por `100` moedas.

### 🔫 Munição & Combate
- **Pente da Arma:** Capacidade de `15` balas engatilhadas.
- **Munição Inicial:** `30` munições totais apenas no início da corrida.
- **Limite Máximo:** `100` munições totais.
- **Munição no Chão:** Adiciona `+10` projéteis.
- **Loja de Munição:** Adiciona `+30` projéteis por `100` moedas.
- **Persistência:** O que sobra continua entre rounds; compras e coletas somam em cima do saldo atual até o limite de `100`.

### ⚡ Baterias (Lanterna & Mapa)
- **Consumo Ativo:** Drena `1%` por segundo de uso + custo de `1%` ao ativar.
- **Carga no Chão:** Recarrega `+15%` em ambos os dispositivos.
- **Loja de Bateria:** Recarrega `+50%` na lanterna e no mapa por `100` moedas, respeitando o limite de `100%`.

### 🪙 Economia & Recompensas
- **Moedas no Chão:** `+25` moedas ao coletar.
- **Bônus de Round:** `+50` moedas por vitória.
- ⚠️ *Atenção: Os recursos coletados no round só são salvos se você vencer a etapa!*

---

## 👾 Inimigos & Perigos

* **🔴 Inimigos Vermelhos:** Realizam patrulhas e perseguem o jogador quando detectado.
* **🩷 Inimigos Rosas:** Atacam à distância assim que alinham visão direta no eixo horizontal ou vertical.
* **🟣 O Chefão (Boss):** Persegue implacavelmente, dispara quando alinhado e ganha aceleração ao se distanciar.
* **💫 Nocaute:** Acerte qualquer inimigo **5 vezes** para desativá-lo temporariamente por **5 segundos**.
* **⚡ Níveis Avançados:** Exigem encontrar a chave dourada para desbloquear a saída, além de introduzir armadilhas, baixa visibilidade e bloqueio de radar.

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

> ℹ️ O pacote `ByteMaze-1.44MB-linux.zip` contém o binário Linux, os assets necessários e este README. Descompactado, fica com aproximadamente `630,264 bytes`, ainda abaixo do limite da competição.

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

> 📌 *Nota: O arquivo de save gerado localmente (`bytemaze_save.dat`) é ignorado e não faz parte do pacote de submissão.*

---

## 👤 Desenvolvedora & Créditos

- **Desenvolvimento:** Sofia A. Figueredo ([@SofiaAFigueredo](https://github.com/SofiaAFigueredo))
- **Assistência técnica:** OpenAI Codex, usado como apoio em revisão de código, balanceamento, correções de build e documentação.
- **Engine Gráfica:** Este projeto utiliza a biblioteca [raylib](https://www.raylib.com/), distribuída sob a licença zlib/libpng. Consulte o arquivo [`raylib/LICENSE`](raylib/LICENSE) para mais detalhes.
