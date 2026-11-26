# 1C. Conflitos shift-reduce

O analisador sintático de Lastro é gerado pelo bison a partir de `src/parser/parser.y` e é um parser LALR(1). Em cada estado do autômato, o parser consulta o próximo token (lookahead) e escolhe uma ação: empilhar o token (shift) ou trocar o lado direito de uma produção, já no topo da pilha, pelo não terminal correspondente (reduce). Um conflito shift-reduce aparece quando, para o mesmo estado e o mesmo token, as duas ações são possíveis. Nesse caso o bison avisa e escolhe o shift, a menos que declarações como `%left` e `%right` digam outra coisa.

A gramática de Lastro tem 202 estados e nenhum conflito. Os conflitos clássicos não foram resolvidos com `%left`/`%right`, foram eliminados por construção: a precedência e a associatividade estão nos níveis da gramática (`expr`, `conj`, `rel`, `soma`, `termo`, `fator`), e as chaves obrigatórias em `if`, `while` e `for` acabam com o dangling else. Para um sistema financeiro, a prioridade de confiabilidade (Problema 1) pede que a ordem de avaliação de `saldo - taxa - juros` esteja escrita na própria gramática e não dependa de uma regra de desempate do gerador. Abaixo mostramos, para cada ponto onde uma gramática ambígua teria conflito, o estado do `build/y.output` em que a decisão é tomada, o trace da derivação e o resultado do binário.

```
make conflitos         # 0 conflitos na gramática real e 1 S/R na variante sem chaves
make test-conflitos    # roda tests/conflitos/: válidos conferem a saída, inválidos a linha do erro
./build/compilador -t < tests/conflitos/valido/01_associatividade.txt   # trace em stderr
```

Os números de estado abaixo valem para a gramática atual. Se a gramática mudar, o estado certo é aquele que contém o item citado, que pode ser localizado com `grep -nF "soma: soma MINUS termo •" build/y.output` (o bison marca a posição do item com `•`).

## Associatividade: `a - b - c`

Numa gramática ambígua como `expr : expr '-' expr`, depois de `10 - 3` com outro `-` à frente o parser poderia reduzir (`(10 - 3) - 2`) ou empilhar (`10 - (3 - 2)`). Em Lastro o operando da direita de `soma MINUS termo` é um `termo`, que não contém `-` binário, então o único caminho é reduzir primeiro. O estado 154 mostra isso:

```
State 154
   93 soma: soma MINUS termo •
   95 termo: termo • STAR fator
   96      | termo • SLASH fator
    STAR   shift, and go to state 129
    SLASH  shift, and go to state 130
    $default  reduce using rule 93 (soma)
```

Com `MINUS` à frente não há shift possível, e o parser reduz pela regra 93. O trace de `10 - 3 - 2` confirma: o segundo `MINUS` só é empilhado depois da redução.

```
Next token is token MINUS ()
Reducing stack by rule 93 (line 399):
   $1 = nterm soma ()
   $2 = token MINUS ()
   $3 = nterm termo ()
-> $$ = nterm soma ()
Entering state 84
Next token is token MINUS ()
Shifting token MINUS ()
```

O C gerado põe parênteses em toda operação e deixa a árvore visível: `escrever_int(((10 - 3) - 2));`. O binário imprime `5`, e não `9`. O programa `tests/conflitos/valido/01_associatividade.txt` faz o mesmo com divisão, onde a diferença também aparece no resultado: `100 / 10 / 5` dá `2` (a associação à direita daria `50`) e `7 / 2 * 2` dá `6` (daria `1`). Como os valores monetários são inteiros (Problema 1), a ordem da divisão inteira muda o valor calculado, e por isso ela não pode ficar ambígua.

## Precedência: `a + b * c`

Depois de `2 + 3` com `*` à frente, uma gramática ambígua teria de escolher entre reduzir (`(2 + 3) * 4`) e empilhar (`2 + (3 * 4)`). Em Lastro, `3` já foi reduzido a `termo`, e o estado 153 tem as duas possibilidades, mas para tokens diferentes:

```
State 153
   92 soma: soma PLUS termo •
   95 termo: termo • STAR fator
   96      | termo • SLASH fator
    STAR   shift, and go to state 129
    SLASH  shift, and go to state 130
    $default  reduce using rule 92 (soma)
```

Com `STAR` o parser empilha, porque o termo ainda pode crescer. Com qualquer outro token ele reduz a soma. O trace de `2 + 3 * 4` mostra o `*` empilhado, a redução de `termo STAR fator` pela regra 95 e só depois a da soma:

```
Next token is token STAR ()
Shifting token STAR ()
...
Reducing stack by rule 95 (line 404):
   $1 = nterm termo ()
   $2 = token STAR ()
   $3 = nterm fator ()
-> $$ = nterm termo ()
Entering state 153
Next token is token RPAREN ()
Reducing stack by rule 92 (line 398):
```

O C gerado é `escrever_int((2 + (3 * 4)));`, e o binário imprime `14`. O mesmo raciocínio vale entre todos os níveis, e `tests/conflitos/valido/02_precedencia.txt` cobre cada um deles: `-2 + 3` dá `1` porque o menos unário está em `fator`; `1 + 2 < 2 * 2` vira `((1 + 2) < (2 * 2))`; `true || false && false` dá `true` porque `&&` liga mais forte que `||`; e `!false && false` dá `false` porque `!` se aplica só ao `fator`. O caso inválido `tests/conflitos/invalido/16_precedencia_semantica.txt` mostra a árvore por outro lado: em `1 + 2 < 4 && 5`, o semântico reclama de `&&` entre `bool` e `int`, o que só acontece se o parser montou `((1 + 2) < 4) && 5`.

Os relacionais ficam num nível sem recursão, `rel : soma op_rel soma`. No estado 84, depois de uma `soma`, o parser empilha o operador relacional; depois da segunda `soma`, no estado 155, só cabem `+` e `-`, e qualquer outro relacional é erro de sintaxe. Assim `1 < 2 < 3`, `a = 1 = c` e `1 < 2 = true` são rejeitados (testes 01 a 03 de `invalido/`), e quem quer comparar o resultado de uma comparação escreve os parênteses, `(1 < 2) = true`. Em C, `1 < 2 < 3` é aceito e compara `(1 < 2)`, que vale 1, com 3, um resultado que quase nunca é o pretendido. Preferimos o erro de compilação.

## `if` aninhado com `else`

O dangling else é o conflito clássico de `if (a) if (b) x; else y;`: depois de `if (b) x;` com `else` à frente, o parser pode reduzir o `if` de dentro sem `else` (o `else` fica com o `if` de fora) ou empilhar o `else` (ele fica com o `if` de dentro). Em Lastro o corpo de um `if` é sempre `bloco`, que termina em `}`, e o `else` só pode vir logo depois desse `}`. No estado 166 a escolha é determinada pelo token:

```
State 166
   73 if_stmt: IF LPAREN expr RPAREN bloco •
   74        | IF LPAREN expr RPAREN bloco • ELSE if_stmt
   75        | IF LPAREN expr RPAREN bloco • ELSE bloco
    ELSE  shift, and go to state 174
    $default  reduce using rule 73 (if_stmt)
```

Não há conflito porque o `if` de dentro está inteiro dentro das chaves do `if` de fora. Quando o `else` aparece, ele está depois do `}` de um único `if`, e é desse `if` que ele é. O programador escolhe o dono pela posição das chaves, e o programa `tests/conflitos/valido/03_if_aninhado.txt` testa os dois casos: com o `else` fora das chaves do `if` interno, ele pertence ao `if` de fora, e o binário não imprime nada quando a condição de fora é verdadeira e a de dentro é falsa; com o `else` dentro das chaves, ele pertence ao `if` interno, e o binário imprime `else interno`. O mesmo programa testa a cadeia `else if` de quatro ramos, que entra pela regra 74. Os casos inválidos 04 a 08 mostram que o `if` sem chaves, o `else` sem bloco, o `else` repetido, o `else` sem `if` e o `else` depois de outro comando são todos rejeitados na linha certa.

### Variante didática: `if` sem chaves

Para mostrar o conflito que as chaves eliminam, `tests/conflitos/dangling_else.y` é uma gramática mínima, fora do compilador, em que o corpo do `if` é um comando qualquer:

```
cmd : IF '(' ID ')' cmd
    | IF '(' ID ')' cmd ELSE cmd
    | ID ';'
    ;
```

O `make conflitos` gera `build/dangling_else.output`, que acusa `State 10 conflicts: 1 shift/reduce`:

```
State 10
    2 cmd: IF '(' ID ')' cmd •
    3    | IF '(' ID ')' cmd • ELSE cmd
    ELSE  shift, and go to state 11
    ELSE      [reduce using rule 2 (cmd)]
    $default  reduce using rule 2 (cmd)
```

A ação entre colchetes é a que o bison descartou. Ele resolveu o conflito com shift, então o `else` fica com o `if` mais próximo, a mesma regra de C e Java. O binário `build/dangling_else` imprime a árvore com chaves explícitas, e o `make test-conflitos` confere que `if (a) if (b) x; else y;` sai como `if(a){if(b){x}else{y}}` e que, com três níveis, cada `else` fica com o `if` aberto mais interno. A resolução está correta, mas é implícita: quem lê o código fonte precisa conhecer a regra para saber de quem é o `else`. Por isso Lastro exige as chaves.

## `ID` seguido de `(`, `[` ou `.`

Um identificador pode começar uma variável (`x`), um elemento de array (`v[i]`), um campo de registro (`r.valor`) ou uma chamada (`f(i)`). Ao ler o `ID`, o parser ainda não sabe qual é, e decide no estado 34 olhando o token seguinte:

```
State 34
   63 alvo: ID •
   66 chamada: ID • LPAREN RPAREN
   67        | ID • LPAREN args RPAREN
    LPAREN  shift, and go to state 69
    $default  reduce using rule 63 (alvo)
```

`(` leva à chamada, e qualquer outro token reduz o `ID` a `alvo`. A indexação e o acesso a campo são decididos depois dessa redução, porque `alvo` é recursivo à esquerda (`alvo LBRACKET expr RBRACKET` e `alvo DOT name`), o que permite encadear `caixa.itens[0].valor`. Dentro de uma expressão, o estado 79 empilha `[` ou `.` e reduz o `alvo` a `fator` com qualquer outro token:

```
State 79
   64 alvo: alvo • LBRACKET expr RBRACKET
   65     | alvo • DOT name
   99 fator: alvo •
    LBRACKET  shift, and go to state 93
    DOT       shift, and go to state 94
    $default  reduce using rule 99 (fator)
```

Nos dois estados cada token leva a uma única ação, então um token de lookahead basta, e não há conflito. O programa `tests/conflitos/valido/04_id_lookahead.txt` combina os casos em `v[dobro(1) - 2]`, `dobro(v[i])` e `dobro(dobro(i)) + v[i] * 2` e confere os valores. A chamada só segue um `ID`, e a indexação só segue um `alvo`, que nunca é uma chamada, então `f(1)[0]` e `v[0](1)` são erros de sintaxe (inválidos 09 e 10). Depois de `ref`, o estado 143 só tem o item `alvo: ID •`, sem a ação para `(`. Em `g(ref f(1))` a ação padrão reduz `f` a `alvo`, o semântico acusa que `f` é um subprograma e não uma variável, e em seguida o `(` dá o erro de sintaxe (inválido 11). Passar por referência o resultado de uma chamada não teria sentido, porque não há variável do chamador para alterar.

## Demais erros

Os testes 12 a 15 de `invalido/` cobrem as expressões incompletas que uma gramática com operadores mal definidos poderia aceitar: `2 + * 3` (o `*` não começa um `termo`), `10 - 3 -` (falta o operando da direita), `true ! false` (o `!` é só prefixo) e `a := b := 1` (a atribuição é comando, não operador). Cada arquivo traz no topo os comentários `// erro:` com as mensagens esperadas, e o `make test-conflitos` confere que o compilador rejeita o programa e que as primeiras mensagens começam, em ordem, com esses textos, inclusive o número da linha.
