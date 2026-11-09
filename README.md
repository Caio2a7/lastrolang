# Problema 7 — DIM0548 Engenharia de Linguagens — Grupo 2

**Integrantes:** Caio Daniel, Flawbert Costa, Murilo Costa e Rodrigo Ferreira.

Implementação do analisador léxico (`src/lexer.l`) e sintático LALR (`src/parser.y`)
da linguagem.

## Build

    make        # gera lex.yy.c, y.tab.c/.h e o binário
    make test   # roda os casos em tests/valido e tests/invalido
    make clean

## Estrutura

- `src/lexer.l`   — especificação léxica
- `src/parser.y`  — gramática BNF não ambígua
- `docs/`         — gramática e resumo das decisões anteriores
- `tests/`        — programas de teste válidos e inválidos

## Decisões que impactam o parser

- Atribuição é `:=` e só existe como comando (nunca dentro de expressão).
- `=` é operador relacional (igualdade), não atribuição.
- Chaves `{ }` obrigatórias em if/while/for — elimina dangling-else por construção.
- Gramática já desambiguada por níveis de não-terminal (expr/conj/rel/soma/termo/fator)
  com recursão à esquerda — operadores binários associam à esquerda sem precisar de
  `%left`/`%right` no yacc.
- Só existem literais inteiros (sem ponto flutuante).

## Detalhes e escolhas de implementação

- **Erro léxico rejeita a entrada:** caractere inválido faz o lexer devolver o token
  `ERRO`, que não aparece em nenhuma produção. Sem isso, o parser poderia aceitar um
  programa depois de um erro léxico. Por isso um erro léxico gera duas mensagens
  (`caractere inválido` e `syntax error`).
- **Mensagens de erro:** `%option yylineno` permite informar a linha e o token onde o
  erro foi detectado. No fim da entrada o token aparece vazio (`perto de ''`).
- **Aceitação:** o yacc adiciona a regra aumentada `S' → S $end`, então `yyparse()` só
  retorna 0 se toda a entrada foi consumida. A `main` apenas imprime `ACEITO` ou `REJEITADO`.
- **Conflitos:** o yacc roda com `-d -v`; o `y.output` mostra os estados do autômato e
  qualquer conflito shift/reduce ou reduce/reduce.
- **Limitações:** sem recuperação de erros (para no primeiro erro) e os lexemas
  copiados com `strdup` não são liberados.
- Arquivos gerados (`lex.yy.c`, `y.tab.*`, `y.output`, binário) estão no `.gitignore`.
