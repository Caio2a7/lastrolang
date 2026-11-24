%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic/semantico.h"
#include "gen/gen.h"

int yylex(void);
int yyerror(char *s);
extern int yylineno;
extern char *yytext;
extern int yydebug;

static Subprog *current_sub;  /* subprograma cuja assinatura/corpo está sendo analisado */
static char *codigo_c = "";   /* programa C inteiro, montado na regra programa */
%}

%union {
    char *str;
    int num;
    TypeDesc *type;
    Atr atr;   /* atributo sintetizado das expressões: (texto C, tipo, linha) */
}

%type <type> tipo tipo_base tipo_prim
%type <num> tamanho

%token <str> ID LITERAL_INT
/* nome que está na tabela de tipos; o léxico decide entre ID e TYPE_ID */
%token <str> TYPE_ID

%type <atr> expr conj rel soma termo fator literal alvo chamada
%type <str> op_rel args arg
%type <str> consts decl_const subprogs subprog corpo bloco bloco_laco fecha_bloco
%type <str> cmds cmd decl_var atrib if_stmt while_stmt for_stmt

%token CONST PROC FUNC INT BOOL IF ELSE WHILE FOR TO
%token SWITCH CASE DEFAULT BREAK CONTINUE RETURN OUT TRUE FALSE

%token ASSIGN EQ NEQ LT LE GT GE
%token PLUS MINUS STAR SLASH
%token AND OR NOT ARROW

%token LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token COMMA SEMI DOT
/* devolvido pelo léxico para caractere inválido; não aparece em nenhuma produção */
%token ERRO

%start programa

%%

programa
    : subprogs          { codigo_c = cod_fmt("%s%s", gen_prologo, $1); }
    | consts subprogs   { codigo_c = cod_fmt("%s%s\n%s", gen_prologo, $1, $2); }
    ;

consts
    : decl_const
    | decl_const consts { $$ = cod_fmt("%s%s", $1, $2); }
    ;

decl_const
    : CONST tipo_prim ID ASSIGN literal SEMI {
            verifica_atribuicao($2, $5.tipo, $5.linha, "constante");
            Symbol *s = declare_var($3, C_CONST, $2, 0, yylineno);
            if (s) s->value = strtol($5.cod, NULL, 10);  /* usado em int[MAX] */
            $$ = cod_fmt("#define %s %s\n", $3, $5.cod);  /* constante de compilação, serve de tamanho */
        }
    ;

subprogs
    : subprog
    | subprog subprogs  { $$ = cod_fmt("%s%s", $1, $2); }
    ;

subprog
    : PROC assinatura corpo             { scope_pop(); $$ = current_sub ? gen_sub(current_sub, $3) : ""; }
    | FUNC assinatura ARROW tipo        {
            if (current_sub) {  /* o escopo aponta para current_sub, então o return já enxerga o tipo */
                current_sub->is_func = 1;
                current_sub->ret = $4;
            }
        }
        corpo                           { scope_pop(); $$ = current_sub ? gen_sub(current_sub, $6) : ""; }
    ;

assinatura
    : nome_sub LPAREN RPAREN
    | nome_sub LPAREN params RPAREN
    ;

nome_sub
    : ID
        {
            current_sub = declare_sub($1, yylineno);  /* antes do corpo: permite recursão */
            scope_push($1, current_sub, 0);
        }
    ;

params
    : param
    | param COMMA params
    ;

param
    : tipo ID       { append_param(current_sub, declare_var($2, C_PARAM, $1, 0, yylineno)); }
    | OUT tipo ID   { append_param(current_sub, declare_var($3, C_PARAM, $2, 1, yylineno)); }
    ;

tipo
    : tipo_base                             { $$ = $1; }
    | tipo_base LBRACKET tamanho RBRACKET   { $$ = type_array($1, $3); }
    ;

tipo_base
    : tipo_prim     { $$ = $1; }
    | TYPE_ID       { $$ = type_lookup($1); }  /* tipo do usuário (#19) */
    ;

tipo_prim
    : INT   { $$ = &type_int; }
    | BOOL  { $$ = &type_bool; }
    ;

/* tamanho de array: literal ou const int, sempre > 0 */
tamanho
    : ID {
            Symbol *s = use_var($1, yylineno);
            $$ = 0;
            if (s && (s->category != C_CONST || s->type != &type_int))
                erro_semantico(yylineno, "tamanho '%s' não é const int", $1);
            else if (s && s->value <= 0)
                erro_semantico(yylineno, "tamanho '%s' tem de ser > 0", $1);
            else if (s)
                $$ = (int)s->value;
        }
    | LITERAL_INT {
            $$ = atoi($1);
            if ($$ <= 0) erro_semantico(yylineno, "tamanho do array tem de ser > 0");
        }
    ;

/* Corpo de subprograma e de for: o escopo já foi aberto antes. */
corpo
    : LBRACE RBRACE         { $$ = ""; }
    | LBRACE cmds RBRACE    { $$ = $2; }
    ;

/* Bloco de if/else: abre um escopo próprio. */
bloco
    : abre_bloco fecha_bloco    { $$ = gen_bloco($2); }
    ;

/* Bloco de while: escopo próprio, marcado como laço. */
bloco_laco
    : abre_laco fecha_bloco     { $$ = gen_bloco($2); }
    ;

abre_bloco
    : LBRACE        { scope_push_block(0); }
    ;

abre_laco
    : LBRACE        { scope_push_block(1); }
    ;

fecha_bloco
    : RBRACE        { scope_pop(); $$ = ""; }
    | cmds RBRACE   { scope_pop(); $$ = $1; }
    ;

cmds
    : cmd
    | cmd cmds  { $$ = cod_fmt("%s%s", $1, $2); }
    ;

cmd
    : decl_var
    | atrib
    | chamada SEMI      { $$ = cod_fmt("%s;\n", $1.cod); }
    | if_stmt
    | while_stmt
    | for_stmt
    | BREAK SEMI        {
            Laco *l = laco_topo();
            if (!l) erro_semantico(yylineno, "break fora de laço");
            $$ = l ? cod_fmt("goto _L%d;\n", l->brk) : "";
        }
    | CONTINUE SEMI     {
            Laco *l = laco_topo();
            if (!l) erro_semantico(yylineno, "continue fora de laço");
            $$ = l ? cod_fmt("goto _L%d;\n", l->cont) : "";
        }
    | RETURN SEMI       {
            int main_proc = current_sub && !current_sub->is_func && strcmp(current_sub->name, "main") == 0;
            $$ = main_proc ? "return 0;\n" : "return;\n";  /* proc main vira int main */
        }
    | RETURN expr SEMI  { $$ = cod_fmt("return %s;\n", $2.cod); }
    ;

/* declare_var depois do inicializador: "int x := x" acusa x não declarado */
decl_var
    : tipo ID ASSIGN expr SEMI  {
            verifica_atribuicao($1, $4.tipo, $4.linha, "inicialização");
            declare_var($2, C_VAR, $1, 0, yylineno);
            $$ = gen_decl($1, $2, $4);
        }
    | tipo ID ASSIGN LBRACKET expr SEMI tamanho RBRACKET SEMI {
            declare_var($2, C_VAR, $1, 0, yylineno);
            if ($1->kind == K_ARRAY) {
                if ($1->size > 0 && $7 > 0 && $7 != $1->size)
                    erro_semantico(yylineno, "'%s' tem %d elementos, mas o inicializador tem %d", $2, $1->size, $7);
                verifica_atribuicao($1->elem, $5.tipo, $5.linha, "inicializador de array");
            }
            $$ = gen_decl_preenche($1, $2, $5, yylineno);
        }
    ;

atrib
    : alvo ASSIGN expr SEMI     {
            if ($1.eh_const)
                erro_semantico($1.linha, "não é possível atribuir à constante '%s'", $1.cod);
            else
                verifica_atribuicao($1.tipo, $3.tipo, $1.linha, "atribuição");
            $$ = gen_atrib($1, $3);
        }
    ;

alvo
    /* tipo vem da tabela de símbolos; não declarado => type_erro (erro já reportado por use_var) */
    : ID    {
            Symbol *s = use_var($1, yylineno);
            /* parâmetro out escalar é ponteiro no C */
            int ptr = s && s->category == C_PARAM && s->is_out && s->type->kind != K_ARRAY;
            $$ = atr(ptr ? cod_fmt("(*%s)", $1) : $1, s ? s->type : &type_erro, yylineno);
            $$.eh_const = s && s->category == C_CONST;
        }
    | ID LBRACKET expr RBRACKET {
            Symbol *s = use_var($1, yylineno);
            $$ = verifica_indexacao($1, s, $3, yylineno);
        }
    ;

/* chamada consulta só a tabela de subprogramas: x() com x variável é erro */
chamada
    : ID LPAREN RPAREN {
            Subprog *p = use_sub($1, yylineno);
            $$ = atr(cod_fmt("%s()", $1), p ? p->ret : &type_erro, yylineno);
        }
    | ID LPAREN args RPAREN {
            Subprog *p = use_sub($1, yylineno);
            $$ = atr(cod_fmt("%s(%s)", $1, $3), p ? p->ret : &type_erro, yylineno);
        }
    ;

args
    : arg
    | arg COMMA args  { $$ = cod_fmt("%s, %s", $1, $3); }
    ;

arg
    : expr     { $$ = $1.cod; }
    /* out = passagem por referência: ponteiro no C; array já é ponteiro */
    | OUT alvo {
            if ($2.eh_const)
                erro_semantico($2.linha, "constante '%s' não pode ser passada como 'out'", $2.cod);
            $$ = $2.tipo->kind == K_ARRAY ? $2.cod : cod_fmt("&%s", $2.cod);
        }
    ;

if_stmt
    : IF LPAREN expr RPAREN bloco                 { $$ = gen_if($3, $5, NULL); }
    | IF LPAREN expr RPAREN bloco ELSE if_stmt    { $$ = gen_if($3, $5, $7); }
    | IF LPAREN expr RPAREN bloco ELSE bloco      { $$ = gen_if($3, $5, $7); }
    ;

/* rótulos do laço criados antes do corpo, para break/continue os acharem */
while_stmt
    : WHILE { laco_abre(); } LPAREN expr RPAREN bloco_laco {
            $$ = gen_while(*laco_topo(), $4, $6);
            laco_fecha();
        }
    ;

for_stmt
    : FOR LPAREN INT ID ASSIGN expr TO expr RPAREN  {
            scope_push_block(1);
            declare_var($4, C_VAR, &type_int, 0, yylineno);
            laco_abre();
        }
        corpo                                       {
            scope_pop();
            $$ = gen_for(*laco_topo(), $4, $6, $8, $11);
            laco_fecha();
        }
    ;

expr
    : expr OR conj  { $$ = atr_bin($1, "||", $3, OP_LOGICO); }
    | conj
    ;

conj
    : conj AND rel  { $$ = atr_bin($1, "&&", $3, OP_LOGICO); }
    | rel
    ;

rel
    : soma op_rel soma  {
            Op op = ($2[0] == '=' || $2[0] == '!') ? OP_IGUALDADE : OP_RELACIONAL;
            $$ = atr_bin($1, $2, $3, op);
        }
    | soma
    ;

op_rel
    : EQ   { $$ = "=="; }  /* Lastro usa '=' para igualdade; C usa '==' */
    | NEQ  { $$ = "!="; }
    | LT   { $$ = "<"; }
    | LE   { $$ = "<="; }
    | GT   { $$ = ">"; }
    | GE   { $$ = ">="; }
    ;

soma
    : soma PLUS termo   { $$ = atr_bin($1, "+", $3, OP_ARITMETICO); }
    | soma MINUS termo  { $$ = atr_bin($1, "-", $3, OP_ARITMETICO); }
    | termo
    ;

termo
    : termo STAR fator   { $$ = atr_bin($1, "*", $3, OP_ARITMETICO); }
    | termo SLASH fator  { $$ = atr_bin($1, "/", $3, OP_ARITMETICO); }
    | fator
    ;

fator
    : literal
    | alvo
    | chamada
    | LPAREN expr RPAREN  { $$ = $2; }  /* atr_bin já pôs os parênteses */
    | MINUS fator         { $$ = atr_un("-", $2, OP_ARITMETICO); }
    | NOT fator           { $$ = atr_un("!", $2, OP_LOGICO); }
    ;

literal
    : LITERAL_INT  { $$ = atr($1, &type_int, yylineno); }
    | TRUE         { $$ = atr(cod_fmt("1"), &type_bool, yylineno); }  /* bool vira int no C */
    | FALSE        { $$ = atr(cod_fmt("0"), &type_bool, yylineno); }
    ;

%%

/* Uso: compilador [-t] [-s] < programa.txt > programa.c
 *   -t  trace da derivação (shift/reduce) em stderr
 *   -s  imprime as tabelas de tipos, subprogramas e símbolos no lugar do C */
int main(int argc, char **argv) {
    int tabelas = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) yydebug = 1;
        else if (strcmp(argv[i], "-s") == 0) tabelas = 1;
    }

    type_init();
    scope_push("global", NULL, 0);
    declare_sub("ler", 0)->is_builtin = 1;
    declare_sub("escrever", 0)->is_builtin = 1;

    int status = yyparse();
    scope_pop();
    if (status == 0 && !sub_lookup("main")) {
        erro_semantico(yylineno, "o programa não tem main");  /* sem ela o gcc não liga */
    }

    if (tabelas) {
        type_print();
        sub_print();
        table_print();
    }

    /* C só sai em stdout se o programa estiver correto; mensagens vão para stderr */
    int erros = erros_semanticos();
    if (status == 0 && erros == 0) {
        if (!tabelas) fputs(codigo_c, stdout);
        fprintf(stderr, "ACEITO: programa correto\n");
        return 0;
    }
    if (erros > 0) {
        fprintf(stderr, "%d erro(s) semântico(s)\n", erros);
    }
    fprintf(stderr, "REJEITADO: programa contém erro\n");
    return 1;
}

int yyerror(char *msg) {
    fprintf(stderr, "linha %d: %s perto de '%s'\n", yylineno, msg, yytext);
    return 0;
}
