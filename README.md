# Lastro

Compilador da linguagem **Lastro**, desenvolvido na disciplina DIM0548 – Engenharia de Linguagens (UFRN/DIMAp). O compilador lê um programa Lastro e o traduz para C; o `gcc` transforma esse C em executável.

**Grupo 2:** Caio Daniel, Flawbert Costa, Murilo Costa e Rodrigo Ferreira.

## A linguagem

Lastro é uma linguagem imperativa, com tipagem estática e subprogramas com passagem de parâmetro por valor ou de saída (`out`).

```
const int MAX := 1000;

func juros(int principal, int taxa_bp, int dias) -> int {
    return (principal * taxa_bp * dias) / (10000 * 360);
}

proc main() {
    int[MAX] v := [0; MAX];
    int total := 0;
    for (int i := 0 to MAX - 1) {
        total := total + juros(100000, 250, i);
    }
    escrever(total);
}
```

- **Tipos:** `int`, `bool` e arrays de tamanho fixo (`int[MAX]`), inicializados com `[valor; tamanho]`.
- **Constantes globais:** `const int MAX := 1000;`
- **Subprogramas:** `proc` (sem retorno) e `func ... -> tipo`. Parâmetros `out` são alterados pelo subprograma chamado e marcados também na chamada: `ler(out n)`.
- **Comandos:** declaração com inicialização obrigatória, atribuição, chamada, `if` / `else if` / `else`, `while`, `for (int i := a to b)`, `break`, `continue` e `return`.
- **Comentários:** `//` até o fim da linha.

### Decisões de projeto

- **Atribuição é `:=`** e só existe como comando, nunca dentro de expressão; `=` é a igualdade.
- **Chaves obrigatórias** em `if`, `while` e `for`, o que elimina o *dangling else* por construção.
- **Precedência por níveis da gramática**, sem `%left`/`%right`: os operadores binários associam à esquerda e os relacionais não se encadeiam.

| Precedência (menor → maior) | Operadores |
|---|---|
| 1 | `\|\|` |
| 2 | `&&` |
| 3 | `=` `!=` `<` `<=` `>` `>=` |
| 4 | `+` `-` |
| 5 | `*` `/` |
| 6 | `-` unário, `!` |

## Como o compilador funciona

```
programa.lastro → analisador léxico → analisador sintático → análise semântica → geração de C → gcc → executável
                  (flex: lexer.l)     (bison: parser.y)       (src/semantic)      (src/gen)
```

1. **Análise léxica (lexical analysis):** `src/lexer/lexer.l` vira a função `yylex()`, que transforma caracteres em tokens.
2. **Análise sintática (parsing):** `src/parser/parser.y` vira a função `yyparse()`, um parser LALR(1) que pede um token por vez ao léxico e confere a estrutura do programa contra a gramática.
3. **Análise semântica (semantic analysis) e geração de código (code generation):** acontecem nas ações semânticas do `parser.y`, numa única passada. Elas usam a tabela de símbolos e a pilha de escopos de `src/semantic/` e montam o código C em `src/gen/`.
4. **gcc:** compila o C gerado em executável.

## Uso

Requisitos: `gcc`, `flex` e `bison` (chamado como `yacc`), e `make`.

```sh
make                                  # gera build/compilador
./build/compilador < programa.txt > programa.c   # traduz para C simplificado
./build/compilador -t < programa.txt  # traduz exibindo o trace da derivação (shift/reduce) em stderr
./build/compilador -s < programa.txt  # imprime as tabelas de tipos, subprogramas e símbolos no lugar do C
make run P=tests/valido/03_funcao_juros.txt  # traduz, compila com gcc -Wall -Werror e executa
make test                             # válidos passam pelo gcc; inválidos têm de dar erro
make conflitos                        # resume os conflitos do autômato LALR (build/y.output)
make clean                            # remove build/
```

Erros são reportados com a linha e o token onde foram detectados, por exemplo `linha 2: syntax error perto de '='`. O código de saída é 0 quando o programa é aceito e 1 quando há erro.

O C só é escrito em stdout quando o programa é aceito; erros e o ACEITO/REJEITADO vão para stderr.

### C simplificado

O C gerado não usa `while`, `for` nem `else`: todo controle de fluxo vira `if (!cond) goto _Ln;` e rótulos `_Ln: ;`. `break` e `continue` são `goto` para os rótulos do laço mais interno. Os demais pontos da tradução:

| Lastro | C gerado |
|---|---|
| `const int MAX := 1000;` | `#define MAX 1000` |
| `bool`, `true`, `false` | `int`, `1`, `0` |
| `:=` / `=` | `=` / `==`, com parênteses em toda operação |
| `proc main()` | `int main(void)`, terminando em `return 0;` |
| `out int x` | `int *x`, usado como `(*x)` e passado como `&x` |
| `int[N] v` sem `out` | cópia local com `memcpy`, então o chamador não muda |
| `[e; n]` | `e` calculado uma vez e laço de preenchimento com `goto` |
| `for (int i := a to b)` | bloco com `i <= b`, `b` calculado uma vez |
| `ler(out x)`, `escrever(e)` | funções embutidas no topo do C, sobre `scanf`/`printf` |

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `src/lexer/` | especificação léxica (flex) |
| `src/parser/` | gramática e ações semânticas (bison) |
| `src/semantic/` | tabela de símbolos e pilha de escopos |
| `tests/valido/`, `tests/invalido/` | programas de teste |
| `docs/` | gramática, documentação e enunciados dos problemas da disciplina |
| `build/` | arquivos gerados pelo `make` (ignorado pelo git) |
| `TASKS.md` | tarefas do compilador, ligadas às issues do repositório |
