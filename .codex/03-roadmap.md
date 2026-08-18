# Roadmap

## Fase 0: Preparacao

- [ ] Criar projeto em `C++` com `raylib`.
- [ ] Confirmar ambiente de compilacao.
- [ ] Escolher resolucao base.
- [ ] Escolher tamanho inicial do tile.

## Fase 1: Prototipo base

- [ ] Abrir janela.
- [ ] Criar loop principal.
- [ ] Implementar grid.
- [ ] Implementar geracao aleatoria do labirinto.
- [ ] Desenhar paredes e caminhos.
- [ ] Inserir jogador.
- [ ] Inserir colisao.
- [ ] Inserir saida.
- [ ] Trocar de nivel ao vencer.

Meta da fase:

- jogo ja pode ser jogado do inicio ao fim de um nivel.

## Fase 2: Progressao

- [ ] Adicionar contador de nivel.
- [ ] Adicionar score.
- [ ] Ajustar aumento de dificuldade basico.
- [ ] Definir tamanho do labirinto por faixa de nivel.

Meta da fase:

- jogo com loop infinito funcional.

## Fase 3: Visao limitada

- [ ] Criar sistema de raio de visao.
- [ ] Definir visao ampla inicial.
- [ ] Reduzir visao a partir do nivel 5.
- [ ] Travar em visao minima nos niveis avancados.

Meta da fase:

- jogo ja tem sua principal identidade.

## Fase 4: Inimigos comuns

- [ ] Criar struct base de inimigo.
- [ ] Fazer spawn a partir do nivel 3.
- [ ] Implementar movimento valido nos corredores.
- [ ] Implementar morte do jogador por toque.
- [ ] Criar diferenciacao entre 3 dificuldades.
- [ ] Ajustar quantidade por nivel.

Meta da fase:

- jogo sai de puzzle e vira sobrevivencia.

## Fase 5: Combate

- [ ] Criar disparo vermelho.
- [ ] Fazer colidir com paredes.
- [ ] Fazer eliminar inimigos comuns.
- [ ] Adicionar pontuacao por eliminacao.

Meta da fase:

- jogador ganha ferramenta de sobrevivencia.

## Fase 6: Inimigo supremo

- [ ] Criar entidade separada.
- [ ] Fazer spawn longe do jogador.
- [ ] Implementar perseguicao.
- [ ] Escalar comportamento com o nivel.
- [ ] Ajustar leitura visual.

Meta da fase:

- jogo ganha pressao inevitavel e ritmo melhor.

## Fase 7: Mapa temporario

- [ ] Detectar nivel 10+.
- [ ] Detectar visao minima.
- [ ] Mostrar mapa por `5` segundos.
- [ ] Exibir jogador, inimigos, supremo e saida.

Meta da fase:

- dar informacao estrategica sem quebrar a tensao.

## Fase 8: Audio e HUD

- [ ] Adicionar som de disparo.
- [ ] Adicionar som de morte do inimigo.
- [ ] Adicionar som de vitoria.
- [ ] Exibir score.
- [ ] Exibir nivel.

Meta da fase:

- feedback basico completo.

## Fase 9: Otimizacao de concurso

- [ ] Medir tamanho do executavel.
- [ ] Medir tamanho total do pacote.
- [ ] Remover tudo o que nao for essencial.
- [ ] Reavaliar necessidade de assets externos.
- [ ] Compilar com foco em tamanho.
- [ ] Testar build final descompactado.

Meta da fase:

- caber em `1.474.560 bytes`.

## Decisoes pendentes

- [ ] Nome final do jogo.
- [ ] Cor final do triangulo do jogador.
- [ ] Cor final do inimigo supremo.
- [ ] Resolucao final.
- [ ] Formato final do mapa temporario.
- [ ] Regra exata de score.
- [ ] Se o inimigo supremo pode ou nao sofrer algum efeito indireto.

## Prioridades absolutas

- [ ] Jogo jogavel.
- [ ] Geracao aleatoria confiavel.
- [ ] Curva de dificuldade clara.
- [ ] Legibilidade visual.
- [ ] Tamanho final dentro da regra.
