# Desenvolvimento passo a passo

Este documento foi feito para voce programar o jogo com ordem, sem travar e sem misturar sistemas cedo demais.

## Ordem ideal de implementacao

Implemente exatamente nesta ordem:

1. Janela e loop principal.
2. Grade do labirinto.
3. Geracao procedural.
4. Desenho do labirinto.
5. Jogador e colisao.
6. Saida do nivel.
7. Sistema de niveis e score.
8. Visao limitada.
9. Inimigos comuns.
10. Disparo.
11. Inimigo supremo.
12. Mapa temporario.
13. HUD minima.
14. Sons.
15. Otimizacao de bytes.
16. Balanceamento final.

## Estrutura recomendada de arquivos

Se quiser manter tudo simples no inicio:

```text
src/
  main.cpp
```

Se o projeto crescer e voce quiser separar melhor:

```text
src/
  main.cpp
  game.h
  game.cpp
  maze.h
  maze.cpp
  entities.h
  entities.cpp
  audio.h
  audio.cpp
```

Comecar com um unico `main.cpp` e totalmente aceitavel. Depois voce separa apenas quando ficar dificil de ler.

## Etapa 1: Janela e loop principal

Objetivo:

- abrir a janela;
- definir `SetTargetFPS`;
- criar o loop de update e draw.

Voce precisa sair desta etapa com:

- uma janela funcionando;
- tela limpa desenhando uma cor de fundo;
- controle de fechamento pelo `WindowShouldClose`.

## Etapa 2: Grade do labirinto

Antes de gerar o labirinto, voce precisa definir como ele existe na memoria.

Forma recomendada:

- usar uma grade `2D`;
- cada celula representa parede ou caminho;
- guardar largura e altura;
- escolher um tamanho fixo de tile em pixels.

Exemplo conceitual:

- `0` = parede;
- `1` = caminho;
- `2` = saida.

## Etapa 3: Geracao procedural do labirinto

Algoritmo mais indicado para este projeto:

- `recursive backtracker` com pilha.

Motivos:

- simples de implementar;
- rapido;
- gera labirintos classicos;
- nao exige bibliotecas externas;
- excelente custo-beneficio para o concurso.

Fluxo:

1. Preencha tudo como parede.
2. Escolha uma celula inicial.
3. Marque como caminho.
4. Enquanto houver caminho para abrir:
5. Escolha um vizinho ainda nao visitado.
6. Quebre a parede entre as duas celulas.
7. Avance.
8. Se travar, volte pela pilha.

Importante:

- use dimensoes impares para o grid;
- deixe entrada e saida em pontos bem separados;
- garanta que o labirinto sempre seja solucionavel.

## Etapa 4: Desenho do labirinto

Desenhe com primitivas da `raylib`:

- `DrawRectangle` para paredes;
- `DrawRectangle` ou fundo para caminho livre.

Padrao visual atual:

- parede preta;
- caminho livre cinza claro.

Nesta etapa, ainda nao precisa de camera complexa. Um top-down fixo resolve.

## Etapa 5: Jogador e colisao

Represente o jogador com:

- posicao em `float`;
- velocidade;
- direcao;
- raio ou caixa de colisao simples.

Para desenhar:

- use `DrawTriangle`.

Para mover:

- leia `WASD` ou setas;
- calcule movimento;
- teste colisao antes de confirmar a nova posicao.

Regra importante:

- o jogador nunca deve atravessar parede;
- movimento deve parecer suave, nao por celula.

## Etapa 6: Saida do nivel

Escolha uma celula distante do inicio e marque como saida.

Quando o jogador tocar a area da saida:

- incrementa nivel;
- soma score;
- gera um novo labirinto;
- reposiciona jogador;
- reposiciona sistemas do nivel.

## Etapa 7: Sistema de niveis e score

Crie uma estrutura simples de estado global:

- nivel atual;
- score;
- tempo do nivel;
- tempo total;
- numero de inimigos derrotados.

Formula inicial sugerida:

- `score += 100 * nivel`
- bonus por tempo rapido;
- bonus por inimigo eliminado.

Comece simples. Balanceamento vem depois.

## Etapa 8: Visao limitada

Essa e uma das mecanicas centrais.

Forma simples de implementar:

1. Desenhe o jogo inteiro em um `RenderTexture`.
2. Cubra a tela com preto.
3. Abra um circulo visivel ao redor do jogador.

Alternativa:

- desenhar um overlay escuro com um recorte circular.

Regras da visao:

- niveis 1 a 4: visao normal ou ampla;
- a partir do nivel 5: reduzir gradualmente o raio de visao;
- em niveis altos: estabilizar na visao minima.

Defina:

- `visionRadiusMax`
- `visionRadiusMin`
- funcao que calcula o raio com base no nivel.

## Etapa 9: Inimigos comuns

Crie uma struct base:

```text
Enemy
- position
- velocity
- type
- speed
- alive
- direction
```

Comportamento inicial recomendado:

- andar pelos corredores;
- escolher direcao valida nas bifurcacoes;
- colidir com paredes;
- matar o jogador por contato.

Nao tente fazer IA complexa cedo demais.

### Como diferenciar as 3 dificuldades

Comece pelo que e barato:

- dificuldade 1: lento;
- dificuldade 2: velocidade media e troca de direcao mais esperta;
- dificuldade 3: rapido e mais insistente ao se aproximar.

Visualmente:

- bola transparente;
- orla branca;
- detalhe de orla diferente por dificuldade.

Exemplos baratos:

- espessura de orla;
- anel extra;
- pequeno marcador interno.

## Etapa 10: Disparo

Se o jogador vai eliminar inimigos, o disparo precisa ser o mais barato possivel em bytes.

Sugestao:

- projetil circular ou pequeno retangulo vermelho;
- velocidade fixa;
- vida curta;
- desaparece na parede;
- elimina inimigo comum ao colidir.

Decisoes importantes:

- o disparo afeta ou nao o inimigo supremo.

Minha recomendacao:

- nao afetar o inimigo supremo;
- isso preserva o papel dele como pressao inevitavel.

## Etapa 11: Inimigo supremo

Crie como sistema separado dos inimigos comuns.

Regras:

- nasce longe do jogador;
- pode usar busca simples de caminho ou perseguicao aproximada;
- aumenta velocidade ou frequencia de decisao conforme o nivel.

Versao mais simples para implementar primeiro:

- recalcular objetivo em intervalos;
- tentar seguir o corredor que mais aproxima do jogador;
- respeitar paredes.

Se ficar caro demais em CPU ou codigo:

- use movimentacao por celulas;
- atualize a rota com menor frequencia.

## Etapa 12: Mapa temporario

A partir do nivel 10:

- se a visao estiver minima;
- mostrar mapa completo por `5` segundos no inicio do nivel.

O mapa pode ser:

- minimapa em um canto;
- ou tela inteira por poucos segundos.

Para economizar implementacao, recomendo:

- minimapa grande sobreposto;
- sem animacoes sofisticadas.

O mapa deve mostrar:

- paredes e caminhos;
- jogador;
- inimigos comuns;
- inimigo supremo;
- saida.

## Etapa 13: HUD minima

Mostre somente o necessario:

- score;
- nivel;
- talvez tempo;
- talvez contador de inimigos.

Para economizar bytes e manter legibilidade:

- use fonte padrao da `raylib` no prototipo;
- so substitua se realmente precisar.

## Etapa 14: Sons

Adicione apenas depois do jogo estar funcional sem audio.

Lista:

- disparo;
- morte do inimigo;
- vitoria.

Dicas:

- mantenha sons curtissimos;
- use baixa taxa quando aceitavel;
- valide o impacto no tamanho final.

## Etapa 15: Otimizacao de bytes

Isso precisa acontecer durante todo o projeto, mas aqui vira foco principal.

Checklist:

- remover assets nao usados;
- evitar fontes externas;
- evitar imagens;
- reduzir ou eliminar menus pesados;
- compilar com flags para tamanho;
- strip no binario;
- testar compressao do pacote final.

## Etapa 16: Balanceamento final

Depois de tudo funcionar:

- ajuste tamanho do labirinto;
- ajuste frequencia de spawn;
- ajuste velocidade dos inimigos;
- ajuste velocidade do supremo;
- ajuste score;
- ajuste curva da visao.

## Ordem detalhada de programacao

Se voce quiser uma trilha ainda mais objetiva, siga esta lista:

1. Fazer janela.
2. Fazer grid.
3. Fazer gerador de labirinto.
4. Desenhar grid.
5. Colocar jogador parado.
6. Fazer jogador mover.
7. Fazer colisao com parede.
8. Colocar saida.
9. Detectar fim do nivel.
10. Reiniciar para outro labirinto.
11. Adicionar contador de nivel.
12. Adicionar score.
13. Implementar raio de visao.
14. Fazer funcao que reduz visao por nivel.
15. Criar vetor de inimigos.
16. Fazer spawn de inimigos apos nivel 2.
17. Fazer inimigos andarem.
18. Fazer contato inimigo-jogador matar.
19. Criar projetil.
20. Fazer disparo colidir com inimigo.
21. Remover inimigo derrotado.
22. Criar inimigo supremo.
23. Fazer spawn longe do jogador.
24. Fazer supremo perseguir.
25. Fazer mapa aparecer por `5` segundos no nivel 10+.
26. Mostrar pontos e nivel na HUD.
27. Adicionar sons.
28. Otimizar tamanho final.
29. Testar varias partidas.
30. Polir para submissao.

## O que evitar cedo demais

- menu complexo;
- particulas demais;
- IA perfeita;
- multiplos modos de jogo;
- muitos tipos de tiro;
- efeitos visuais caros;
- assets externos sem necessidade.

## Meta tecnica ideal

Seu primeiro grande objetivo nao e terminar o jogo inteiro. E chegar em um prototipo jogavel com:

- labirinto aleatorio;
- jogador;
- saida;
- loop de nivel;
- score.

Depois disso, todo o resto entra como camada.
