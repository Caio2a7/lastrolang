# Tarefas

Ordem de execução do compilador, da mais urgente para a menos urgente. Cada tarefa tem uma issue no GitHub com o contexto, a checklist e o critério de pronto. Prazo das duas entregas (implementação e documentação): 01/12/2026 14h30.

**Critério:** código do que a tarefa atende; a legenda está no fim do arquivo.

## Prioridade alta: implementação

Os problemas do `problemas_avaliacao.pdf` dependem destas tarefas. Fazer nesta ordem.

**[#17](https://github.com/Caio2a7/lastro/issues/17) Adicionar matrizes**\
Sintaxe do `programas/03_matrizes.txt`: tipo `int[MAX][MAX] a`, acesso `a[i][j]` (inclusive `ler(ref a[i][j])`) e inicializador `[0; MAX, MAX]`. Matriz como parâmetro, por valor e `ref`. Pronto quando o Problema 3 traduz e roda.\
Área: sintático, semântico, geração de C. Critério: Eliminatório.

**[#19](https://github.com/Caio2a7/lastro/issues/19) Adicionar registros e tipos definidos pelo usuário**\
Declaração de tipo com campos, acesso com `.`, registro como parâmetro (valor e `ref`), como retorno de `func` e atribuição entre registros do mesmo tipo. Pronto quando o Problema 4 (`rational_t`) traduz e roda.\
Área: léxico, sintático, semântico, geração de C. Critério: Eliminatório.

**[#12](https://github.com/Caio2a7/lastro/issues/12) Verificar as faixas de índices**\
Índice constante fora do tamanho do array dá erro em compilação; índice que é expressão ganha uma verificação no C gerado.\
Área: semântico, geração de C. Critério: Doc. 2E.

**[#14](https://github.com/Caio2a7/lastro/issues/14) Verificar o controle de fluxo**\
Condição `bool` em `if`/`while`, `break`/`continue` só dentro de laço e o `main` tem de existir.\
Área: semântico. Critério: Doc. 2E.

## Prioridade alta: programas e testes

**[#22](https://github.com/Caio2a7/lastro/issues/22) Escrever os programas do problemas_avaliacao.pdf**\
Os 6 problemas, escritos na linguagem e traduzidos sem erro. Hoje o 01 e o 05 são aceitos; o 03 depende da #17, o 04 da #19, o 06 da #21, e o 02 ainda não foi escrito.\
Área: testes. Critério: Eliminatório.

**[#21](https://github.com/Caio2a7/lastro/issues/21) Escrever o programa da árvore binária de busca (Problema 6)**\
Árvore sobre um array de registros (filhos como índices, `-1` = nulo), sem ponteiros nem `malloc`. Inserção, mínimo e máximo com o nível, e impressão nível a nível com uma fila feita sobre array. Depende da #17 e da #19.\
Área: testes. Critério: Eliminatório.

**[#28](https://github.com/Caio2a7/lastro/issues/28) Criar a suíte de testes ponta a ponta**\
`make test` completo: `.txt` → `.c` → `gcc` → execução, comparando a saída com a esperada. Inclui `programas/`, para nenhum programa da avaliação entrar na `main` sem compilar.\
Área: testes. Critério: Eliminatório.

**[#23](https://github.com/Caio2a7/lastro/issues/23) Testar erros léxicos e sintáticos**\
Um programa inválido para cada tipo de erro.\
Área: testes. Critério: 1A.

**[#24](https://github.com/Caio2a7/lastro/issues/24) Demonstrar os conflitos shift-reduce**\
A gramática atual tem 0 conflitos por construção. Programas com `a - b - c`, `a + b * c` e `if` aninhado com `else` provam que associatividade e precedência saem corretas; uma variante da gramática com `if` sem chaves mostra o conflito e que o yacc resolve com shift.\
Área: testes, documentação. Critério: 1C.

**[#25](https://github.com/Caio2a7/lastro/issues/25) Testar escopos de variáveis**\
Variável omitida e duplicada no mesmo escopo dão erro; nomes repetidos em escopos irmãos e aninhados são aceitos, conferindo qual valor é acessado (inclusive variável local com o nome de uma `const` global).\
Área: testes. Critério: 2A–2D.

**[#26](https://github.com/Caio2a7/lastro/issues/26) Testar expressões e coerções**\
Coerções permitidas, com o cast aparecendo no C, e misturas de tipos incompatíveis que precisam dar erro.\
Área: testes. Critério: 3A, 3B.

**[#27](https://github.com/Caio2a7/lastro/issues/27) Testar a passagem de parâmetros**\
Imprimir o valor antes e depois da chamada, com e sem `ref`, executando o binário.\
Área: testes. Critério: 4A, 4B.

## Prioridade alta: documentação

Entra no PDF da entrega.

**[#29](https://github.com/Caio2a7/lastro/issues/29) Documentar a apresentação da linguagem**\
Tipos, comandos, subprogramas e a gramática BNF final (também em `docs/gramatica-bnf.md`).\
Área: documentação. Critério: Doc. 1.

**[#31](https://github.com/Caio2a7/lastro/issues/31) Documentar a tabela de símbolos e os escopos**\
Tabela de símbolos e pilha de escopos (texto 3.1/3.2 do grupo, com os nomes únicos de bloco e o shadowing).\
Área: documentação. Critério: Doc. 2B.

**[#32](https://github.com/Caio2a7/lastro/issues/32) Documentar condicionais e repetição**\
Gramática, escopo do `for` e tradução para C.\
Área: documentação. Critério: Doc. 2C.

**[#33](https://github.com/Caio2a7/lastro/issues/33) Documentar os subprogramas**\
Cabeçalho, recursão, `ref` e retorno.\
Área: documentação. Critério: Doc. 2D.

**[#34](https://github.com/Caio2a7/lastro/issues/34) Documentar as verificações semânticas**\
Tipos, coerções, faixas, duplicidade, chamadas e fluxo.\
Área: documentação. Critério: Doc. 2E.

**[#35](https://github.com/Caio2a7/lastro/issues/35) Documentar as instruções de uso e montar o PDF final**\
Build, compilar um programa, gerar C e rodar. Atualizar o README e corrigir o `==` que aparece em `docs/documentacao.pdf`.\
Área: documentação. Critério: Doc. 3.

## Prioridade baixa

**[#16](https://github.com/Caio2a7/lastro/issues/16) Adicionar o operador de potência**\
Operador novo entre `termo` e `fator`, associativo à direita. No C vira `pot_int`/`pow`. Nenhum problema da avaliação exige.\
Área: léxico, sintático, semântico, geração de C.

## Concluídas

**[#2](https://github.com/Caio2a7/lastro/issues/2) Corrigir o erro léxico que não rejeita o programa**\
O `return ERRO` voltou ao léxico; um programa com `@` sai REJEITADO.\
Área: léxico. Critério: 1A.

**[#3](https://github.com/Caio2a7/lastro/issues/3) Criar os atributos semânticos e o relatório de erros**\
Na `%union`, o atributo guarda (código C, tipo). `erro_semantico(linha, msg)` conta o erro sem abortar.\
Área: sintático.

**[#4](https://github.com/Caio2a7/lastro/issues/4) Adicionar o modo trace da derivação**\
Opção `-t` (`yydebug`) que imprime os shifts e reduces, para mostrar a derivação na apresentação.\
Área: sintático. Critério: 1B.

**[#5](https://github.com/Caio2a7/lastro/issues/5) Implementar as tabelas de símbolos, de subprogramas e de tipos**\
Três tabelas sobre uma tabela hash genérica: símbolos (chave `escopo#nome`), subprogramas (chave `nome`, com retorno e parâmetros em ordem) e tipos (chave `nome`, descritor `TypeDesc`). As entradas não são apagadas no `pop`.\
Área: semântico.

**[#6](https://github.com/Caio2a7/lastro/issues/6) Implementar a pilha de escopos**\
Pilha de escopos (scope stack): cada nível guarda (escopo, subprograma dono, se é laço) e qualquer nível pode ser lido. Cada bloco recebe um nome único (`main#block2`).\
Área: semântico.

**[#7](https://github.com/Caio2a7/lastro/issues/7) Abrir e fechar escopos na gramática**\
`nome_sub` registra o subprograma antes do corpo (assim a recursão funciona) e insere os parâmetros; `push`/`pop` em bloco e no `for`; `ler`/`escrever` registrados como embutidos.\
Área: sintático, semântico. Critério: 2C, 2D.

**[#8](https://github.com/Caio2a7/lastro/issues/8) Verificar as declarações**\
Erro para nome omitido, duplicado no mesmo escopo e tamanho de array que não é `const int`.\
Área: semântico. Critério: 2A, 2B.

**[#9](https://github.com/Caio2a7/lastro/issues/9) Verificar os tipos das expressões**\
Verificador de tipos (type checking): aritmética só com números, lógica só com `bool`, relacionais resultam em `bool`, `=`/`!=` com tipos compatíveis.\
Área: semântico. Critério: 3B.

**[#10](https://github.com/Caio2a7/lastro/issues/10) Verificar atribuição e indexação**\
Tipo compatível, com coerção (coercion) só de `int` para `float`. Proibido atribuir a `const`. O nome indexado tem de ser array e o índice `int`. `[e; n]` coerente com o tipo.\
Área: semântico. Critério: 3B.

**[#11](https://github.com/Caio2a7/lastro/issues/11) Adicionar o tipo float e a coerção de int para float**\
Literal real no léxico, `float` em `tipo_prim` e coerção de `int` para `float` em expressões, atribuições, `return` e argumentos, visível no C como `((double)x)`. O Problema 1 é aceito.\
Área: léxico, sintático, semântico, geração de C. Critério: 3A.

**[#13](https://github.com/Caio2a7/lastro/issues/13) Verificar chamadas de subprograma e retorno**\
O subprograma existe, número e tipo dos argumentos conferem e `ref` bate entre chamada e parâmetro. `proc` não pode aparecer em expressão. `return` coerente com `proc` ou `func`.\
Área: semântico. Critério: 4A.

**[#15](https://github.com/Caio2a7/lastro/issues/15) Gerar o código C**\
Todas as construções, com saída em stdout: `./build/compilador < x.txt > x.c && gcc -Wall -Werror x.c`. `ref` vira ponteiro, então o valor muda depois da chamada; parâmetro sem `ref` não muda.\
Área: geração de C. Critério: 4B.

**[#30](https://github.com/Caio2a7/lastro/issues/30) Documentar a análise léxica**\
Tokens, casamento mais longo (longest match) e erros. O texto saiu do relatório e volta reescrito na seção de léxico (#29), em `relatorio/secoes/02-1-lexico.tex`.\
Área: documentação. Critério: Doc. 2A.

**[#48](https://github.com/Caio2a7/lastro/issues/48) Escrever texto, float e bool com escrever**\
`escrever` imprime `int`, `float`, `bool` (`true`/`false`) e texto entre aspas; `ler` aceita `ref float`. Texto não é tipo: só existe como argumento de `escrever`.\
Área: léxico, sintático, semântico, geração de C. Critério: Eliminatório.

**[#49](https://github.com/Caio2a7/lastro/issues/49) Permitir shadowing em escopos aninhados**\
Uma declaração só conflita com o escopo do topo; num escopo de dentro, o nome esconde o de fora até o bloco fechar. Constante vira `static const` no C.\
Área: semântico. Critério: 2D.

## Fora do escopo

**[#18](https://github.com/Caio2a7/lastro/issues/18) Adicionar o comando switch**\
Fechada: nenhum problema da avaliação nem dica do professor exige.

**[#20](https://github.com/Caio2a7/lastro/issues/20) Adicionar referências e alocação dinâmica**\
Fechada: a árvore do Problema 6 usa array de registros (#21).

## Legenda dos critérios

**Avaliação do professor**

- **1A:** Detecção de erros léxicos e sintáticos
- **1B:** Derivação pelas regras sintáticas
- **1C:** Conflito shift-reduce e ações corretas
- **2A:** Variáveis omitidas
- **2B:** Variáveis duplicadas no mesmo escopo
- **2C:** Variáveis duplicadas em escopos não aninhados
- **2D:** Variáveis duplicadas em escopos aninhados
- **3A:** Coerções permitidas
- **3B:** Tipos incompatíveis
- **4A:** Parâmetros transmitidos corretamente
- **4B:** Parâmetros alterados ou inalterados após o retorno
- **Eliminatório:** programas traduzidos sem erro para C

**Itens do enunciado da documentação**

- **Doc. 1:** Apresentação da linguagem
- **Doc. 2A:** Unidades léxicas
- **Doc. 2B:** Símbolos e tabela de símbolos
- **Doc. 2C:** Condicionais e repetição
- **Doc. 2D:** Subprogramas
- **Doc. 2E:** Verificações realizadas
- **Doc. 3:** Instruções de uso
