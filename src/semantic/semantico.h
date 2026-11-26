#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "semantic/types.h"    /* TypeDesc; &type_erro propaga em silêncio */
#include "semantic/symtab.h"
#include "semantic/subprog.h"

/* Categoria do operador, usada para verificar e decidir o tipo do resultado. */
typedef enum { OP_ARITMETICO, OP_RELACIONAL, OP_IGUALDADE, OP_LOGICO } Op;

/* Atributo sintetizado de uma expressão: texto C, tipo, linha e se é constante. */
typedef struct {
    char *cod;
    TypeDesc *tipo;
    int linha;
    int eh_const;
} Atr;

/* Monta um atributo a partir de seus três componentes (eh_const = 0). */
Atr atr(char *cod, TypeDesc *tipo, int linha);

/* Operação binária: cod "(a op b)", verifica tipos por compativel/tipo_resultado, linha de a. */
Atr atr_bin(Atr a, const char *op_c, Atr b, Op op);

/* Operação unária: cod "(op a)"; os parênteses evitam "- -x" virar "--x" em C. */
Atr atr_un(const char *op_c, Atr a, Op op);

/* Compatibilidade de operandos em expressões: mesmo tipo (exceto void) ou int <-> float.
 * Grava em *res o tipo resultante (type_float em int/float, type_erro se algum for erro). */
int compativel(TypeDesc *a, TypeDesc *b, TypeDesc **res);

/* Tipo resultante de um operador, ou &type_erro se os operandos forem incompatíveis. */
TypeDesc *tipo_resultado(Op op, TypeDesc *a, TypeDesc *b);

/* 1 se 'origem' pode ser atribuído a 'destino' (type_equal, int -> float, ou type_erro).
 * float -> int e void são rejeitados. */
int tipo_atribuivel(const TypeDesc *destino, const TypeDesc *origem);

/* Coerção int -> float: se destino é float e e é int, devolve "((double)e)" com tipo float.
 * Nos demais casos devolve e sem mudança. */
Atr coage(Atr e, const TypeDesc *destino);

/* Verifica atribuição/inicialização; reporta erro em 'linha' se incompatível. */
int verifica_atribuicao(const TypeDesc *destino, const TypeDesc *origem, int linha, const char *ctx);

/* Verifica indexação base[idx]: exige array e índice int; devolve o atributo do elemento.
 * Matriz é array de array, então a[i][j] é (a[i])[j]: mais índices que dimensões cai em "não é array". */
Atr verifica_indexacao(Atr base, Atr idx, int linha);

/* Verifica [e; n1, ..., nk] em 'nome' do tipo t. forma traz os k tamanhos (array de void):
 * têm de ser as k primeiras dimensões de t, e e tem de caber no que sobra.
 * Devolve k, ou 0 se não houver o que gerar (t não é array ou há dimensões demais). */
int verifica_preenche(const char *nome, const TypeDesc *t, const TypeDesc *forma, Atr e, int linha);

/* Argumento de chamada em lista encadeada, na ordem da esquerda para a direita. */
typedef struct Arg {
    char *cod;
    TypeDesc *tipo;
    int is_ref;
    int linha;
    struct Arg *next;
} Arg;

/* Cria um nó de argumento para a lista de argumentos de uma chamada. */
Arg *arg_novo(char *cod, TypeDesc *tipo, int is_ref, int linha);

/* Verifica chamada de subprograma (existência, aridade, tipos, ref e embutidos). */
Atr verifica_chamada(const char *nome, Arg *args, int linha);

/* Rejeita uso de procedimento (retorno void) dentro de expressão. */
Atr verifica_chamada_expr(Atr ch);

/* Verifica comando return em proc (sem valor) e func (com valor compatível). */
void verifica_retorno(Subprog *sub, int tem_expr, Atr expr, int linha);

/* printf que devolve texto C em memória recém-alocada. */
char *cod_fmt(const char *fmt, ...);

/* Reporta erro semântico em stderr e segue a análise (não aborta). */
void erro_semantico(int linha, const char *fmt, ...);

/* Quantidade de erros semânticos reportados até agora. */
int erros_semanticos(void);

/* ---------- Declarações e usos de nomes (consultam as três tabelas) ---------- */

/* 1 se o nome pode ser declarado agora: não está no escopo do topo (escopos de fora
 * podem ter o mesmo nome: shadowing), nem é subprograma, nem tipo.
 * Se não puder, reporta o erro e devolve 0. */
int nome_livre(const char *nome, int linha);

/* Declara variável, constante ou parâmetro no escopo do topo; NULL se o nome não estiver livre. */
Symbol *declare_var(const char *name, Category category, TypeDesc *type, int is_ref, int line);

/* Símbolo visível com esse nome; NULL (com erro reportado) se não houver. */
Symbol *use_var(const char *name, int line);

/* Declara um subprograma (começa como proc; a regra de func ajusta is_func e ret). */
Subprog *declare_sub(const char *name, int line);

/* Subprograma com esse nome; NULL (com erro reportado) se não houver. */
Subprog *use_sub(const char *name, int line);

/* Acrescenta o parâmetro ao fim da lista do subprograma e conta n_params. */
void append_param(Subprog *sub, Symbol *param);

#endif
