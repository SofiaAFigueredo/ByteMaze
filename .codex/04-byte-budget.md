# Orcamento de bytes

O maior risco deste projeto e o tamanho final do jogo, nao a logica.

## Limite oficial

- Tamanho maximo descompactado: `1.474.560 bytes`

## Estrategia geral

Tudo o que puder ser feito por codigo deve ser feito por codigo.

Isso vale para:

- personagens;
- inimigos;
- paredes;
- mapa;
- efeitos visuais simples;
- parte da interface.

## Metas de custo

Estas metas nao sao regras fixas. Sao guias para manter o projeto seguro.

- Executavel principal: `700 KB` a `1100 KB`
- Audio total: `50 KB` a `180 KB`
- Assets extras restantes: `0 KB` a `120 KB`
- Folga de seguranca: `150 KB` ou mais

## O que quase sempre vale a pena

- primitivas da `raylib`;
- poucos sons curtos;
- uma unica fonte padrao no prototipo;
- nenhum arquivo de imagem, se possivel;
- reutilizacao maxima de sistemas.

## O que deve ser tratado com cuidado

- fontes externas;
- texturas;
- efeitos com transparencia pesada;
- bibliotecas extras;
- menus grandes;
- varias trilhas ou muitos efeitos sonoros.

## Riscos especificos de `C++ + raylib`

- O executavel pode crescer mais do que o esperado.
- O runtime conta no tamanho final.
- Dependendo da forma de compilacao, o binario sozinho pode consumir grande parte do limite.

## Plano de mitigacao

1. Prototipar primeiro sem assets externos.
2. Medir o tamanho do binario cedo.
3. Medir de novo sempre que entrar audio ou novos recursos.
4. Remover tudo o que nao impacta diretamente a jogabilidade.
5. Priorizar jogo pequeno e polido em vez de jogo grande e apertado no limite.

## Regras de aprovacao para novas features

Antes de adicionar qualquer recurso novo, pergunte:

- melhora claramente a experiencia?
- custa poucos bytes?
- consegue ser feito com codigo em vez de asset?
- complica muito o balanceamento?

Se a resposta for "nao" em varios itens, corte.

## Assets atuais planejados

- Sem imagens obrigatorias.
- Sem sprites obrigatorios.
- Sem trilha sonora obrigatoria.
- Apenas `3` efeitos sonoros curtos.

## Check de submissao

Antes de submeter:

- [ ] medir executavel;
- [ ] medir pacote total descompactado;
- [ ] remover arquivos temporarios;
- [ ] remover arquivos de debug;
- [ ] confirmar que tudo necessario esta incluso;
- [ ] confirmar que nada inutil ficou junto.
