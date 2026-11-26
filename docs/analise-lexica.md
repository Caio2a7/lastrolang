# 2A. Unidades léxicas

O analisador léxico de Lastro fica em `src/lexer/lexer.l`, e o flex o transforma na função `yylex()`. Quem comanda a leitura é o analisador sintático, que pede um token por vez. A cada pedido, o léxico lê a entrada padrão até fechar o próximo lexema e devolve o código do token. Se o token tem valor, como um identificador ou um número, o lexema vai para `yylval.str`. A opção `%option yylineno` guarda a linha corrente, e todas as mensagens de erro do compilador usam esse número, sejam elas léxicas, sintáticas ou semânticas. O léxico consulta a tabela de tipos para classificar identificadores. As ações do parser cadastram as declarações nas tabelas.

## Tokens e expressões regulares

A especificação parte de três definições auxiliares: `digito = [0-9]`, `letra = [a-zA-Z]` e `ident = {letra}({letra}|{digito}|_)*`. A tabela abaixo lista os tokens na mesma ordem em que as regras aparecem no arquivo.

| Classe | Lexema ou expressão regular | Token |
|---|---|---|
| Palavra reservada | `const` `proc` `func` `type` | `CONST` `PROC` `FUNC` `TYPE` |
| Palavra reservada | `int` `float` `bool` | `INT` `FLOAT` `BOOL` |
| Palavra reservada | `if` `else` `while` `for` `to` | `IF` `ELSE` `WHILE` `FOR` `TO` |
| Palavra reservada | `switch` `case` `default` | `SWITCH` `CASE` `DEFAULT` |
| Palavra reservada | `break` `continue` `return` `ref` | `BREAK` `CONTINUE` `RETURN` `REF` |
| Literal lógico | `true` `false` | `TRUE` `FALSE` |
| Atribuição | `:=` | `ASSIGN` |
| Relacional | `=` `!=` `<=` `<` `>=` `>` | `EQ` `NEQ` `LE` `LT` `GE` `GT` |
| Aritmético | `+` `-` `*` `/` | `PLUS` `MINUS` `STAR` `SLASH` |
| Lógico | `&&` `\|\|` `!` | `AND` `OR` `NOT` |
| Tipo de retorno | `->` | `ARROW` |
| Delimitador | `(` `)` `[` `]` `{` `}` | `LPAREN` `RPAREN` `LBRACKET` `RBRACKET` `LBRACE` `RBRACE` |
| Separador | `,` `;` `.` | `COMMA` `SEMI` `DOT` |
| Literal de texto | `\"[^\"\\\n]*\"` | `LITERAL_TEXTO` |
| Texto sem fechar | `\"[^\"\\\n]*` | `ERRO` |
| Literal real | `{digito}+"."{digito}+` | `LITERAL_FLOAT` |
| Literal inteiro | `{digito}+` | `LITERAL_INT` |
| Identificador | `{ident}` | `TYPE_ID` para nomes cadastrados na tabela de tipos, `ID` nos demais casos |
| Erro | `.` (qualquer outro caractere) | `ERRO` |

A linguagem tem 21 palavras reservadas, todas em minúsculas. Como ela diferencia maiúsculas de minúsculas, `If` e `WHILE` são identificadores comuns. `ler` e `escrever` também são identificadores: chegam ao parser como `ID` e entram na tabela de símbolos como subprogramas embutidos, de modo que a análise semântica confere uma chamada a eles do mesmo jeito que confere qualquer outra.

Todo identificador começa com uma letra ASCII e continua com letras, dígitos ou sublinhado. Deixamos as letras acentuadas de fora porque o código de sistemas financeiros conversa com bibliotecas C e com sistemas legados (Problema 1), e um nome como `preço` não teria equivalente direto no C gerado. O literal inteiro é uma sequência de dígitos, sem sinal. Em `-5`, o `-` é o operador unário da gramática. O literal real exige dígitos dos dois lados do ponto (`2.5`; `2.` e `.5` não são aceitos). Em `2.5`, as regras de inteiro e de real casam ao mesmo tempo, e o casamento mais longo escolhe `LITERAL_FLOAT`. Valores monetários continuam sendo guardados como inteiros (em centavos ou pontos-base), como decidido no Problema 1. O `float` existe para os cálculos não monetários e para a coerção de `int` para `float` exigida na avaliação.

O literal de texto fica entre aspas, na mesma linha e sem `\`, e vai para o C exatamente como foi escrito. A linguagem não tem tipo `string`: o texto só é aceito como argumento de `escrever`, para as mensagens exigidas nos problemas de avaliação. Quando as aspas não fecham na linha, ou quando aparece uma `\`, só a regra de texto sem fechar casa, e o léxico reporta `texto sem aspas de fechamento`.

O parser usa `TYPE` na declaração de registros e `DOT` no acesso a seus campos. O léxico também reconhece `SWITCH`, `CASE` e `DEFAULT`, que seguem reservados para o comando `switch`.

## Casamento mais longo e palavras reservadas antes de identificador

O flex desempata com duas regras. Pela primeira, o casamento mais longo (longest match), vence a expressão que consome mais caracteres a partir da posição atual. Se duas expressões casam o mesmo número de caracteres, vale a segunda regra: ganha a que foi declarada primeiro no arquivo.

Com o casamento mais longo, os operadores que começam igual não precisam de tratamento especial. Em `<=`, tanto `"<="` quanto `"<"` casam, mas a primeira leva dois caracteres e ganha. O mesmo vale para `:=` e `=`, `>=` e `>`, `!=` e `!`, `->` e `-`, e por isso a ordem dessas regras no arquivo não muda nada. O comentário segue a mesma lógica: `"//".*` sempre consome mais que `"/"`, então duas barras seguidas nunca viram duas divisões.

Nas palavras reservadas, quem decide é a ordem. `while` casa `"while"` e também `{ident}`, com o mesmo comprimento, e é lido como palavra reservada porque sua regra vem antes da regra de identificador. Se a ordem fosse invertida, o léxico nunca devolveria uma palavra reservada. O casamento mais longo cobre o caso inverso. Em `ifx`, a regra `"if"` casa dois caracteres e `{ident}` casa três, então `ifx` é um identificador, e o programador pode dar a uma variável um nome que começa com uma palavra reservada.

## Comentários, espaços e erro léxico

O léxico reconhece comentários e espaços e os descarta sem gerar token. O comentário vai de `//` até o fim da linha (`"//".*`), porque o `.` do flex não casa a quebra de linha. A linguagem não tem comentário de bloco, então ninguém esquece um comentário aberto pelo caminho, e cada linha pode ser lida sozinha, o que atende à prioridade de legibilidade do Problema 1. A expressão `[ \t\r\n]+` cobre os espaços, que só separam lexemas. O `\r` está ali para aceitar arquivos com quebra de linha do Windows. As quebras de linha não geram token, mas o `yylineno` as conta.

A última regra, `.`, pega qualquer caractere que nenhuma outra regra aceitou: `@`, `#`, um `:` sozinho, um `&` ou `|` sozinho, ou cada byte de uma letra acentuada. A ação chama `yyerror("caractere inválido")`, que imprime a linha e o lexema, e devolve o token `ERRO`. Nenhuma produção usa esse token, então o parser rejeita o programa no mesmo ponto e o compilador sai com código 1. Com `tests/invalido/05_caractere_invalido.txt`, a saída é:

```
linha 3: caractere inválido perto de '@'
linha 3: syntax error perto de '@'
REJEITADO: programa contém erro
```

Dois dos erros previstos no Problema 6 formam sequências de tokens válidas e por isso passam pelo léxico. Quem os detecta é o parser. O literal fracionário incompleto `10.` vira `LITERAL_INT DOT`, e a mensagem é `syntax error perto de '.'`. O literal mal formado `3x` vira `LITERAL_INT ID`, com a mensagem `syntax error perto de 'x'`. Nos dois casos o programa é rejeitado na linha certa.

## Limitações

O parser usa a recuperação de erro padrão do yacc e para no primeiro erro de sintaxe. Como todo `ERRO` provoca um erro de sintaxe, o compilador relata só o primeiro caractere inválido do programa. O Problema 6 previa que o léxico seguisse adiante depois de um erro, e a implementação atual não faz isso. O léxico também não confere a faixa do literal inteiro: `99999999999999999999` passa e só daria problema ao compilar o C gerado. As duas questões ficam como pendências do compilador.
