# Guia Iniciante do ByteMaze

Este arquivo existe para te ajudar a construir o jogo aos poucos, sem precisar entender tudo de uma vez.

A ideia aqui e simples:

- explicar o projeto como se voce estivesse comecando do zero;
- ensinar uma mecanica por vez;
- evitar jogar o codigo inteiro de uma vez em cima de voce;
- mostrar o que fazer, por que fazer e como testar;
- lembrar sempre do limite de `1,44 MB`.

Se alguma parte parecer "obvia demais", tudo bem. Este guia foi feito para nao pular etapas.

---

## Como usar este guia

Nao tente fazer tudo no mesmo dia.

Use assim:

1. Leia somente a mecanica atual.
2. Entenda o objetivo daquela parte.
3. Escreva apenas o codigo daquela parte.
4. Teste.
5. So passe para a proxima quando a atual estiver funcionando.

Regra importante:

- se uma etapa nao funciona, nao avance;
- se voce avanca com erro pequeno, ele vira erro grande depois.

---

## O que e programar, em linguagem simples

Programar e escrever instrucoes para o computador.

Essas instrucoes dizem coisas como:

- abrir uma janela;
- desenhar um quadrado;
- mover o jogador;
- detectar se bateu na parede;
- trocar de nivel.

O computador nao "adivinha" nada.
Voce precisa dizer tudo de forma clara.

Exemplos de ideias basicas:

- **variavel**: uma caixinha que guarda um valor;
- **funcao**: um bloco de instrucoes com uma tarefa especifica;
- **loop**: algo que repete sem parar enquanto o jogo estiver aberto;
- **condicao**: um "se isso acontecer, faca aquilo";
- **struct**: um grupo de dados que pertencem a uma mesma coisa.

Exemplo mental:

- o jogador tem `posicao`, `velocidade` e `tamanho`;
- essas tres informacoes podem morar dentro de uma `struct Player`.

---

## O que e `1,44 MB` e por que isso importa

O concurso limita o jogo inteiro a `1.474.560 bytes`.

Isso e o mesmo que aproximadamente `1,44 MB`.

Em termos simples:

- seu jogo nao pode crescer sem controle;
- cada imagem, som, fonte e biblioteca pesa;
- ate o executavel final pesa;
- se passar do limite, o jogo pode ficar invalido para o concurso.

### O que pesa no tamanho final

Estas coisas costumam aumentar o tamanho:

- imagens;
- sprites;
- musicas;
- muitos efeitos sonoros;
- fontes externas;
- bibliotecas extras;
- builds mal otimizadas.

### O que ajuda a economizar espaco

Estas coisas ajudam:

- desenhar formas por codigo;
- usar a fonte padrao no comeco;
- usar poucos sons;
- evitar arquivos externos;
- reaproveitar sistemas simples.

### Traduzindo isso para o ByteMaze

Por isso a ideia do projeto usa:

- triangulo para o jogador;
- circulos para inimigos;
- quadrado para o inimigo supremo;
- paredes desenhadas com retangulos;
- audio minimo;
- pouca dependencia de assets.

Em resumo:

- menos arquivo pronto;
- mais coisa gerada por codigo.

---

## O que voce vai construir primeiro

Seu primeiro objetivo nao e "terminar o jogo".

Seu primeiro objetivo e conseguir isto:

1. abrir uma janela;
2. gerar um labirinto;
3. mover o jogador;
4. impedir que ele atravesse parede;
5. colocar uma saida;
6. trocar de nivel.

Se isso funcionar, o resto vira camadas em cima de uma base boa.

---

## Ordem recomendada

Siga exatamente esta ordem:

1. Janela e loop principal
2. Grid do labirinto
3. Geracao procedural
4. Desenho do labirinto
5. Jogador
6. Colisao com parede
7. Saida do nivel
8. Nivel e score
9. Visao limitada
10. Inimigos comuns
11. Disparo
12. Inimigo supremo
13. Mapa temporario
14. HUD
15. Sons
16. Otimizacao de tamanho

---

## Mecanica 1: Janela e loop principal

### O que isso significa

Antes de existir jogo, precisa existir uma janela.

Essa janela e onde tudo sera desenhado.

Depois disso, o jogo precisa repetir dois trabalhos o tempo todo:

- atualizar o estado do jogo;
- desenhar o estado do jogo.

Esse ciclo se chama `game loop`.

### O que voce precisa entender

- o jogo fica rodando varias vezes por segundo;
- em cada repeticao, ele le teclado, atualiza posicoes e desenha;
- isso continua ate a janela ser fechada.

### O que escrever nesta etapa

Nesta fase, faca apenas isto:

- iniciar a janela;
- definir FPS;
- criar o `while` principal;
- limpar a tela com uma cor;
- fechar corretamente.

### O que testar

So avance se:

- a janela abrir;
- a tela aparecer limpa;
- o programa fechar sem travar.

### Erros comuns

- esquecer de criar o loop;
- desenhar fora de `BeginDrawing` e `EndDrawing`;
- tentar criar mecanicas cedo demais.

### Impacto no tamanho

Quase nenhum impacto relevante alem da propria base do executavel.

---

## Mecanica 2: Grid do labirinto

### O que e um grid

Um grid e uma tabela.

Pense em um caderno quadriculado:

- cada quadradinho e uma celula;
- cada celula pode ser parede, caminho ou saida.

### Por que usar grid

Porque labirintos funcionam muito bem com grade.

Fica facil:

- gerar caminhos;
- detectar parede;
- posicionar inimigos;
- achar saida;
- desenhar tudo.

### Como pensar a estrutura

Voce precisa guardar:

- largura do grid;
- altura do grid;
- tamanho do tile em pixels;
- conteudo de cada celula.

### Representacao simples

Uma forma boa:

- `0` = parede;
- `1` = caminho;
- `2` = saida.

### O que escrever nesta etapa

Crie:

- constantes do tamanho do grid;
- constante do tamanho do tile;
- uma estrutura de matriz `2D`;
- uma forma de preencher tudo com parede.

### O que testar

So avance se:

- a grade existir na memoria;
- voce conseguir preencher tudo com parede;
- voce conseguir ler uma celula especifica.

### Erros comuns

- misturar pixel com celula;
- nao saber se uma coordenada e de grid ou de tela;
- usar grid muito grande cedo demais.

### Dica de iniciante

Sempre diferencie estas duas coisas na sua cabeca:

- coordenada de **grid**: `x = 5`, `y = 7`;
- coordenada de **pixel**: `x = 160`, `y = 224`.

---

## Mecanica 3: Geracao procedural do labirinto

### O que significa "procedural"

Significa que o labirinto nao vem pronto em arquivo.

O proprio codigo cria um labirinto novo quando o nivel comeca.

### Algoritmo recomendado

Use `recursive backtracker` com pilha.

Isso parece nome dificil, mas a ideia e simples:

1. comece com tudo fechado;
2. escolha um ponto inicial;
3. abra caminho;
4. escolha um vizinho ainda nao visitado;
5. abra a parede entre eles;
6. continue;
7. se travar, volte para tras;
8. repita ate terminar.

### Por que esse algoritmo e bom aqui

Porque ele:

- e relativamente simples;
- funciona bem para labirinto classico;
- nao precisa de biblioteca extra;
- cabe melhor na ideia de projeto enxuto.

### O que voce precisa entender antes de programar

O algoritmo trabalha melhor quando:

- o grid tem largura impar;
- o grid tem altura impar;
- as celulas "visitaveis" ficam separadas por paredes.

Por isso, muitas vezes ele anda de `2 em 2` no grid.

### O que escrever nesta etapa

Voce vai precisar:

- de uma pilha;
- de uma celula inicial;
- de uma lista de vizinhos possiveis;
- de um jeito de quebrar a parede entre duas celulas.

### O que testar

So avance se:

- o labirinto nao sair todo fechado;
- o labirinto nao sair todo aberto;
- o jogador conseguir ter um caminho real entre areas;
- o labirinto parecer diferente a cada rodada.

### Erros comuns

- esquecer de marcar celula como visitada;
- quebrar parede errada;
- usar coordenadas pares e impares sem criterio;
- gerar saida em lugar inacessivel.

### Dica muito importante

Antes de pensar em inimigo, som ou score, garanta:

- o labirinto nasce certo;
- ele e solucionavel.

Se o labirinto estiver ruim, todo o resto fica ruim.

---

## Mecanica 4: Desenho do labirinto

### O que esta acontecendo aqui

Ate agora o labirinto existia na memoria.

Agora voce vai transformar isso em imagem na tela.

### Como pensar

Cada celula do grid vira um retangulo desenhado na tela.

Exemplo:

- se a celula for parede, desenha preto;
- se for caminho, desenha cinza claro;
- se for saida, desenha outra cor temporaria.

### O que escrever nesta etapa

Voce vai fazer:

- dois loops, um para `y` e um para `x`;
- ler cada celula do grid;
- escolher uma cor;
- desenhar um retangulo naquele tile.

### O que testar

So avance se:

- o labirinto aparece visualmente;
- as paredes e corredores ficam legiveis;
- cada nova geracao muda o desenho.

### Erros comuns

- desenhar tudo na mesma posicao;
- esquecer de multiplicar por `TILE_SIZE`;
- usar cores sem contraste.

### Dica visual

Neste projeto, legibilidade vale mais que beleza complexa.

Se der para enxergar bem:

- parede;
- corredor;
- saida;

entao voce esta no caminho certo.

---

## Mecanica 5: Jogador

### O que o jogador precisa ter

Mesmo um jogador simples precisa de algumas informacoes.

Por exemplo:

- posicao `x`;
- posicao `y`;
- velocidade;
- direcao;
- tamanho para colisao.

### O que significa "posicao em float"

`float` e um numero com parte decimal.

Isso ajuda o movimento a ficar suave.

Exemplo:

- com inteiro, voce pisaria de `100` para `101`;
- com `float`, pode ir de `100.0` para `100.2`, `100.4`, `100.6`.

### Como desenhar o jogador

O projeto pede um triangulo.

Entao voce vai:

- guardar a posicao central;
- calcular tres pontos;
- desenhar com `DrawTriangle`.

### O que escrever nesta etapa

Crie:

- uma `struct Player`;
- posicao inicial;
- velocidade;
- leitura do teclado;
- atualizacao da posicao.

### O que testar

So avance se:

- o jogador aparecer;
- o teclado mover;
- o movimento parecer suave.

### Erros comuns

- movimentar sem usar `delta time`;
- misturar velocidade com posicao;
- mover por celula em vez de mover livremente.

### O que e `delta time`

E o tempo que passou desde o ultimo frame.

Sem isso:

- em computador rapido o jogador corre demais;
- em computador lento o jogador anda devagar.

Com isso:

- o movimento fica mais consistente.

---

## Mecanica 6: Colisao com parede

### O que e colisao

Colisao e a regra que impede coisas de atravessarem outras coisas.

Aqui, a regra mais importante e:

- o jogador nao pode passar pela parede.

### Forma simples de pensar

Antes de confirmar a nova posicao do jogador:

1. calcule onde ele iria;
2. descubra em qual celula isso cai;
3. veja se e parede;
4. se for parede, cancele o movimento;
5. se nao for, aceite o movimento.

### O que escrever nesta etapa

Voce precisa:

- calcular `nextPosition`;
- converter essa posicao para coordenada de grid;
- consultar a celula;
- aplicar ou bloquear.

### O que testar

So avance se:

- o jogador parar na parede;
- ele ainda conseguir andar pelos corredores;
- nao ficar tremendo a cada toque.

### Erros comuns

- testar colisao usando coordenada errada;
- deixar o jogador entrar meio tile na parede;
- aplicar movimento antes de validar.

### Dica de evolucao

No comeco, faca colisao simples.

Depois, se quiser melhorar:

- teste eixo `x` e `y` separadamente;
- use raio do jogador;
- ajuste deslizamento em cantos.

Nao tente isso cedo demais.

---

## Mecanica 7: Saida do nivel

### O que e a saida

A saida e a condicao de vitoria de cada round.

Quando o jogador encosta nela:

- o nivel termina;
- um novo labirinto e criado;
- o jogo continua.

### Como pensar essa mecanica

Voce precisa de duas coisas:

- uma celula marcada como saida;
- uma verificacao de contato entre jogador e essa area.

### O que escrever nesta etapa

Voce vai:

- escolher uma celula distante do inicio;
- marcar essa celula como `EXIT`;
- detectar quando o jogador entra nela;
- chamar a logica de proximo nivel.

### O que acontece ao vencer o round

No minimo:

- aumenta o nivel;
- soma score;
- reinicia tempo do nivel;
- gera novo labirinto;
- recoloca o jogador;
- reseta coisas daquele nivel.

### O que testar

So avance se:

- a saida estiver visivel;
- tocar nela realmente trocar o nivel;
- o proximo labirinto nascer corretamente.

### Erros comuns

- gerar saida perto demais do inicio;
- nao resetar o jogador;
- manter lixo do nivel anterior.

---

## Mecanica 8: Nivel e score

### O que e nivel

Nivel e a forma mais simples de medir progresso.

Quanto mais niveis o jogador vence:

- maior o desafio;
- maior a tensao;
- maior a pontuacao possivel.

### O que e score

Score e a pontuacao do jogador.

Ele serve para recompensar:

- avancar;
- jogar bem;
- jogar rapido;
- eliminar ameacas.

### O que escrever nesta etapa

Crie um estado do jogo com:

- nivel atual;
- score atual;
- tempo do nivel;
- tempo total;
- inimigos derrotados.

### Formula simples para comecar

Uma ideia inicial:

- pontos por completar nivel;
- bonus por rapidez;
- bonus por derrotar inimigos.

Mas no comeco, faca o minimo:

- `score += 100 * nivel`

Depois voce ajusta.

### O que testar

So avance se:

- o nivel sobe;
- o score muda;
- os valores nao resetam sem motivo.

### Erros comuns

- score crescer no momento errado;
- nivel nao atualizar antes da nova rodada;
- misturar score permanente com score temporario.

---

## Mecanica 9: Visao limitada

### Por que essa mecanica existe

Essa e uma das identidades mais fortes do jogo.

No inicio, o jogador enxerga mais.

Depois, ele enxerga menos e precisa:

- memorizar caminhos;
- tomar decisoes mais rapidas;
- lidar com a tensao.

### O que acontece na pratica

Voce desenha o jogo inteiro.

Depois, cobre a tela com escuridao.

Em seguida, deixa visivel so uma area ao redor do jogador.

### O que voce precisa controlar

Pelo menos:

- raio maximo de visao;
- raio minimo de visao;
- nivel a partir do qual a visao encolhe;
- formula que reduz esse raio.

### Regra do projeto

- niveis 1 a 4: visao ampla;
- nivel 5 em diante: visao comeca a cair;
- niveis altos: trava no minimo.

### O que escrever nesta etapa

Voce vai criar:

- valores de visao maxima e minima;
- funcao que calcula o raio pelo nivel;
- overlay escuro;
- abertura circular sobre o jogador.

### O que testar

So avance se:

- no inicio o mapa ficar legivel;
- depois a visao realmente diminuir;
- o jogador continuar conseguindo jogar.

### Erros comuns

- deixar escuro demais cedo demais;
- reduzir a visao sem limite minimo;
- fazer a visao sumir completamente.

### Dica de design

A mecanica precisa gerar tensao, nao injustica.

Se o jogador nao enxerga nada e morre sem entender por que, a experiencia piora.

---

## Mecanica 10: Inimigos comuns

### O papel deles

Esses inimigos existem para transformar o jogo em sobrevivencia, nao so exploracao.

Eles criam risco local:

- corredor perigoso;
- perseguicao curta;
- pressao enquanto o jogador procura a saida.

### Estrutura simples

Cada inimigo pode ter:

- posicao;
- velocidade;
- direcao;
- tipo;
- estado vivo ou morto.

### Comportamento inicial bom para comecar

Faca simples:

- anda pelos corredores;
- escolhe direcao nas bifurcacoes;
- respeita parede;
- mata o jogador por toque.

### Tres dificuldades

Voce nao precisa de IA super complexa.

Diferencie assim:

- tipo 1: lento;
- tipo 2: medio;
- tipo 3: rapido.

Depois adicione pequenos comportamentos extras, se ainda couber.

### O que escrever nesta etapa

Voce vai criar:

- `struct Enemy`;
- vetor de inimigos;
- funcao de spawn;
- funcao de update;
- funcao de desenho;
- deteccao de toque no jogador.

### O que testar

So avance se:

- inimigos aparecem;
- se movem sem atravessar parede;
- matar por contato funciona;
- diferencas entre tipos sao perceptiveis.

### Erros comuns

- spawnar em cima do jogador;
- spawnar dentro da parede;
- IA complexa cedo demais;
- muitos inimigos logo de cara.

### Dica de economia de bytes

Mudanca de velocidade e desenho simples custam bem menos do que IA elaborada.

---

## Mecanica 11: Disparo

### Para que serve

O disparo da ao jogador uma ferramenta de sobrevivencia.

Ele nao resolve tudo.
Ele so ajuda a abrir espaco.

### Como manter barato em bytes

O disparo deve ser:

- pequeno;
- simples;
- rapido;
- descartavel.

### Estrutura simples

Cada tiro pode ter:

- posicao;
- direcao;
- velocidade;
- tempo de vida;
- estado ativo.

### Regras boas para comecar

- nasce na frente do jogador;
- anda reto;
- morre ao bater na parede;
- elimina inimigo comum;
- nao afeta o inimigo supremo.

### O que escrever nesta etapa

Crie:

- `struct Bullet`;
- vetor de tiros;
- funcao de spawn;
- update de movimento;
- teste com parede;
- teste com inimigo;
- remocao de tiro gasto.

### O que testar

So avance se:

- apertar o botao cria tiro;
- o tiro anda;
- ele some na parede;
- ele remove inimigo comum.

### Erros comuns

- tiro nascer dentro da parede;
- tiro nao ter limite de vida;
- tiro atravessar tudo;
- tiro eliminar o que nao deveria.

---

## Mecanica 12: Inimigo supremo

### Por que ele existe

Esse inimigo evita que a partida vire um passeio lento.

Ele e a pressao inevitavel do jogo.

Sua funcao e:

- empurrar o ritmo;
- impedir demora excessiva;
- criar medo constante.

### Como ele deve ser diferente

Ele nao e so "mais um inimigo".

Ele precisa:

- nascer longe do jogador;
- perseguir de forma consistente;
- crescer em eficiencia com o nivel;
- ser facil de identificar visualmente.

### Como implementar sem exagero

Comece simples:

- recalcular alvo de tempos em tempos;
- seguir o corredor que parece aproximar do jogador;
- respeitar paredes.

Nao tente uma busca perfeita cedo demais.

### O que escrever nesta etapa

Crie:

- uma entidade separada;
- regra de spawn distante;
- update de perseguicao;
- regra de velocidade por nivel;
- desenho proprio.

### O que testar

So avance se:

- ele nascer longe;
- realmente perseguir;
- ficar mais perigoso com o progresso;
- ser legivel mesmo com pouca visao.

### Erros comuns

- spawnar perto demais;
- ser rapido demais cedo;
- ficar preso com frequencia;
- parecer igual aos outros inimigos.

---

## Mecanica 13: Mapa temporario

### Por que isso entra mais tarde

Quando a visao ficar muito pequena, o jogo pode ficar cruel demais.

Entao o mapa temporario entra como ajuda estrategica.

### Regra do projeto

A partir do nivel `10`:

- se a visao ja estiver no minimo;
- mostre o mapa por `5` segundos no inicio do nivel.

### O que esse mapa precisa mostrar

- paredes;
- caminhos;
- jogador;
- inimigos comuns;
- inimigo supremo;
- saida.

### Forma simples de fazer

A melhor abordagem para comecar e:

- sobrepor um minimapa grande;
- sem animacao sofisticada;
- mostrar por tempo curto;
- esconder depois.

### O que escrever nesta etapa

Voce vai criar:

- temporizador de exibicao;
- condicao de nivel alto;
- desenho do mapa completo;
- elementos principais em escala reduzida.

### O que testar

So avance se:

- ele aparecer no inicio do nivel certo;
- sumir depois de 5 segundos;
- realmente ajudar sem acabar com a tensao.

### Erros comuns

- mostrar em todos os niveis;
- nao esconder depois;
- deixar grande demais ou pequeno demais.

---

## Mecanica 14: HUD

### O que e HUD

HUD e a informacao desenhada na tela para o jogador.

Aqui, menos e mais.

### O que mostrar

No minimo:

- nivel;
- score.

Opcionalmente:

- tempo;
- contador de inimigos.

### O que escrever nesta etapa

Desenhe texto simples com:

- fonte padrao;
- posicao fixa;
- informacao realmente util.

### O que testar

So avance se:

- a leitura estiver facil;
- o texto nao atrapalhar a jogabilidade;
- os numeros atualizarem corretamente.

### Erros comuns

- muita informacao na tela;
- fonte externa desnecessaria;
- posicao ruim em cima do jogo.

### Dica de tamanho

Nao invente UI grande cedo.

HUD minima e mais barata e mais segura para o concurso.

---

## Mecanica 15: Sons

### Quando adicionar

Som entra so depois que o jogo ja funciona sem ele.

Se o jogo depende de som para parecer bom, o nucleo ainda esta fraco.

### Sons planejados

Somente tres:

- disparo;
- morte do inimigo;
- vitoria.

### O que escrever nesta etapa

Voce precisa:

- iniciar sistema de audio;
- carregar ou gerar sons curtos;
- tocar cada som no evento certo.

### O que testar

So avance se:

- os sons tocam no evento correto;
- nao ha atraso estranho;
- o tamanho final ainda esta controlado.

### Erros comuns

- usar arquivos longos;
- colocar som demais;
- nao medir impacto no build final.

---

## Mecanica 16: Otimizacao de tamanho

### O que isso quer dizer

Aqui voce para de olhar so para "funciona ou nao funciona" e passa a olhar:

- quanto pesa;
- quanto da para cortar;
- se cada coisa vale o espaco que ocupa.

### Perguntas que voce deve fazer para cada recurso

- melhora mesmo a experiencia?
- custa poucos bytes?
- da para fazer por codigo?
- complica muito o jogo?

Se varias respostas forem "nao", corte.

### O que revisar

Cheque:

- imagens externas;
- fontes externas;
- audios grandes;
- codigo inutil;
- arquivos temporarios;
- arquivos de debug;
- flags de compilacao.

### Metas de seguranca

Os documentos do projeto sugerem algo assim:

- executavel principal: `700 KB` a `1100 KB`
- audio total: `50 KB` a `180 KB`
- extras restantes: `0 KB` a `120 KB`
- folga: `150 KB` ou mais

Isso nao e lei absoluta.
E uma faixa de seguranca.

### O que testar

So considere pronto se:

- o executavel foi medido;
- o pacote total foi medido;
- o jogo ainda funciona depois dos cortes.

---

## O que NAO fazer cedo demais

Evite isto no comeco:

- menu complexo;
- varios modos de jogo;
- IA perfeita;
- muitos tipos de tiro;
- particulas demais;
- efeitos visuais caros;
- assets externos sem necessidade.

Essas coisas costumam:

- atrasar o desenvolvimento;
- gerar mais bugs;
- aumentar o tamanho final.

---

## Como saber se voce esta indo bem

Voce esta indo bem se:

1. cada etapa funciona antes da proxima;
2. voce entende o que escreveu;
3. o jogo fica jogavel cedo;
4. o projeto continua leve;
5. novas features entram por necessidade, nao por impulso.

---

## Roteiro de estudo pratico

Se voce estiver muito perdida, siga esta rotina:

### Dia 1

- entender janela;
- entender loop;
- abrir tela limpa.

### Dia 2

- criar grid;
- preencher com parede;
- desenhar blocos na tela.

### Dia 3

- gerar labirinto procedural;
- testar varias geracoes.

### Dia 4

- criar jogador;
- mover com teclado;
- fazer colisao.

### Dia 5

- criar saida;
- trocar de nivel;
- adicionar score simples.

### Dia 6 em diante

- visao limitada;
- inimigos comuns;
- disparo;
- inimigo supremo;
- mapa temporario;
- HUD;
- sons;
- otimizacao.

---

## Se voce travar

Quando travar, faca estas perguntas:

1. O erro esta na logica ou no desenho?
2. O valor que eu usei esta certo?
3. Estou confundindo pixel com celula?
4. Estou tentando fazer duas mecanicas ao mesmo tempo?
5. Eu testei a menor versao possivel dessa ideia?

Quase sempre o bloqueio vem de uma destas causas:

- etapa grande demais;
- falta de teste pequeno;
- misturar varios sistemas cedo demais.

---

## Regra final deste guia

Nao tente impressionar o computador.

Tente ser clara.

Codigo bom para este projeto e codigo que:

- voce entende;
- funciona;
- cabe no limite;
- pode crescer sem virar bagunca.

Se quiser continuar estudando a partir daqui, a melhor proxima acao e implementar somente a **Mecanica 1: Janela e loop principal** e parar ali ate funcionar.
