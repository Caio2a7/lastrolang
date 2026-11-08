# Problema 7 — DIM0548 Engenharia de Linguagens — Grupo 2

Caio Daniel, Flawbert Costa, Murilo Costa, Pedro Paiva e Rodrigo Ferreira.

Implementação do analisador léxico (`src/lexer.l`) e sintático (`src/parser.y`)
da linguagem definida nos Problemas 1 a 6, usando lex/yacc clássico.

## Build

    make        # gera lex.yy.c, y.tab.c/.h e o binário
    make test   # roda os casos em tests/valido e tests/invalido
    make clean

## Estrutura

- `src/lexer.l`   — especificação léxica (Problema05, item 1.2)
- `src/parser.y`  — gramática BNF não ambígua (Problema05, item 4.2)
- `docs/`         — gramática e resumo das decisões anteriores
- `tests/`        — programas de teste válidos e inválidos
- `examples-professor/` — exemplo de referência fornecido em aula (lexer02.l / parser02.y / input02.txt)

## Decisões que impactam o parser

- Atribuição é `:=` e só existe como comando (nunca dentro de expressão).
- `=` é operador relacional (igualdade), não atribuição.
- Chaves `{ }` obrigatórias em if/while/for — elimina dangling-else por construção.
- Gramática já desambiguada por níveis de não-terminal (expr/conj/rel/soma/termo/fator)
  com recursão à esquerda — operadores binários associam à esquerda sem precisar de
  `%left`/`%right` no yacc.
- Só existem literais inteiros (sem ponto flutuante).
