# Visao do jogo

## Resumo

Jogo de labirinto infinito com geracao procedural, visao limitada e pressao crescente.

O jogador controla um triangulo e deve encontrar a saida de cada nivel. Conforme avanca, a visao fica menor, novos inimigos entram em cena e um inimigo supremo passa a perseguir o jogador.

## Objetivo do jogador

- Sobreviver.
- Encontrar a saida.
- Avancar o maximo de niveis possivel.
- Fazer o maior score possivel.

## Fantasia central

No inicio o jogador sente controle. Com o tempo, ele perde visao, ganha mais informacao para memorizar e passa a ser pressionado por perseguidores. O jogo deve criar a sensacao de:

- exploracao;
- tensao;
- memoria espacial;
- risco crescente;
- sobrevivencia em rounds curtos.

## Identidade visual

- Jogador: triangulo.
- Inimigos comuns: circulos.
- Inimigo supremo: quadrado.
- Paredes: preto.
- Chao ou caminho livre: cinza claro.
- Disparo do jogador: vermelho.
- Visao minima: circulo claro ao redor do jogador.

## Identidade sonora

Somente tres sons principais para preservar bytes:

- disparo;
- morte do inimigo;
- vitoria.

## Regras de progressao

### Niveis 1 e 2

- Apenas aprender a navegar.
- Sem excesso de pressao.
- Foco em movimento, colisao, saida e score base.

### A partir do nivel 3

- Inimigos comuns aparecem.
- Tres dificuldades devem existir e aumentar com o tempo.

### A partir do nivel 5

- A visao do jogador passa a diminuir gradualmente.

### A partir do nivel 10

- Se a visao estiver na forma minima, o mapa aparece por `5` segundos no inicio do nivel.

### Niveis avancados

- O inimigo supremo comeca longe do jogador.
- Ele deve perseguir lentamente.
- Sua eficiencia deve subir com o progresso.

## Inimigos comuns

Precisam ser simples o bastante para caber no projeto e diferentes o bastante para criar variedade.

### Dificuldade 1

- Movimento lento.
- Patrulha simples ou aleatoria.
- Serve para ensinar o perigo.

### Dificuldade 2

- Movimento medio.
- Reage melhor a proximidade do jogador.
- Pode mudar de direcao com mais frequencia.

### Dificuldade 3

- Movimento rapido.
- Mais agressivo.
- Menor margem de erro para o jogador.

## Inimigo supremo

Funcao principal:

- impedir partidas lentas demais;
- criar tensao inevitavel;
- transformar o jogo em sobrevivencia e nao apenas em puzzle.

Regras desejadas:

- nasce longe do jogador;
- nao aparece colado na saida;
- anda mais devagar no inicio;
- escala de forma controlada por nivel;
- deve ser legivel visualmente, mesmo com pouca visao.

## Score

O score precisa incentivar jogo bom, nao apenas jogo longo.

Componentes sugeridos:

- pontos por completar nivel;
- bonus por velocidade;
- bonus por eliminar inimigos;
- multiplicador por nivel alto.

## Condicao de derrota

O jogador perde quando:

- for tocado por um inimigo;
- for alcancado pelo inimigo supremo;
- ou quando voce decidir adicionar vida unica sem continues.

## Condicao de vitoria do round

- Alcancar a saida do labirinto.

## Principios de design

- Nada deve depender de arte pesada.
- Tudo importante deve ser legivel por forma e cor.
- O nucleo precisa ser divertido mesmo sem som.
- O jogo deve funcionar com assets minimos.
- Toda feature nova deve justificar seu custo em bytes.
