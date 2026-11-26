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
    Arg *arg;  /* lista de argumentos de uma chamada */
    Field *campo;  /* lista de campos de um tipo do usuário */
}

%type <type> tipo tipo_base tipo_prim tamanhos
%type <num> tamanho
%type <str> name decl_type
%type <campo> fields field
%type <arg> record_values

%token <str> ID LITERAL_INT LITERAL_FLOAT LITERAL_TEXTO
/* nome que está na tabela de tipos; o léxico decide entre ID e TYPE_ID */
%token <str> TYPE_ID

%type <atr> expr conj rel soma termo fator literal alvo chamada
%type <arg> args arg
%type <str> op_rel
%type <str> globais global decl_const subprogs subprog corpo bloco bloco_laco fecha_bloco
%type <str> cmds cmd decl_var atrib if_stmt while_stmt for_stmt

%token CONST PROC FUNC INT FLOAT BOOL IF ELSE WHILE FOR TO TYPE NULO NOVO CARET
%token SWITCH CASE DEFAULT BREAK CONTINUE RETURN REF TRUE FALSE

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
    | globais subprogs  { codigo_c = cod_fmt("%s%s\n%s", gen_prologo, $1, $2); }
    ;

globais
    : global
    | global globais { $$ = cod_fmt("%s%s", $1, $2); }
    ;

global
    : decl_const
    | decl_type
    ;

decl_const
    : CONST tipo_prim ID ASSIGN literal SEMI {
            verifica_atribuicao($2, $5.tipo, $5.linha, "constante");
            Symbol *s = declare_var($3, C_CONST, $2, 0, yylineno);
            if (s) s->value = strtol($5.cod, NULL, 10);  /* usado em int[MAX] */
            /* static const e não #define: o #define trocaria também uma variável local de mesmo nome
             * (shadowing). Tamanhos de array saem como número no C, então não dependem do nome. */
            $$ = cod_fmt("static const %s %s = %s;\n", c_tipo($2), $3, coage($5, $2).cod);
        }
    ;

decl_type
    /* o tipo entra na tabela antes do '{': dentro dos campos o nome já é TYPE_ID */
    : TYPE name { $<type>$ = declare_type_abre($2, yylineno); } LBRACE fields RBRACE SEMI {
            TypeDesc *t = declare_type_fecha($<type>3, $5, yylineno);
            $$ = t ? gen_decl_type(t) : "";
        }
    ;

name
    : ID
    | TYPE_ID
    ;

fields
    : field
    | field fields { $1->next = $2; $$ = $1; }
    ;

field
    : tipo name SEMI {
            Field *f = malloc(sizeof *f);
            if (!f) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            *f = (Field){ .name = $2, .type = $1 };
            $$ = f;
        }
    ;

record_values
    : expr {
        $$ = arg_novo($1.cod, $1.tipo, 0, $1.linha);
    }
    | expr COMMA record_values {
        $$ = arg_novo($1.cod, $1.tipo, 0, $1.linha);
        $$->next = $3;
    }
    ;

subprogs
    : subprog
    | subprog subprogs  { $$ = cod_fmt("%s%s", $1, $2); }
    ;

subprog
    : PROC assinatura corpo             { scope_pop(); $$ = current_sub ? gen_sub(current_sub, $3) : ""; }
    | FUNC assinatura ARROW tipo        {
            if ($4->kind == K_ARRAY) {
                erro_semantico(yylineno, "função não pode retornar array; use parâmetro 'ref'");
            }
            if (current_sub) {  /* o escopo aponta para current_sub, então o return já enxerga o tipo */
                current_sub->is_func = 1;
                current_sub->ret = $4->kind == K_ARRAY ? &type_erro : $4;
            }
        }
        corpo                           {
            if (current_sub && !current_sub->has_return) {
                erro_semantico(yylineno, "função '%s' não possui comando return", current_sub->name);
            }
            scope_pop();
            $$ = current_sub ? gen_sub(current_sub, $6) : "";
        }
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
    | REF tipo ID   { append_param(current_sub, declare_var($3, C_PARAM, $2, 1, yylineno)); }
    ;

tipo
    : tipo_base                         { $$ = $1; }
    | tipo LBRACKET tamanho RBRACKET    { $$ = type_add_dim($1, $3); }  /* matriz: int[N][M] */
    ;

tipo_base
    : tipo_prim       { $$ = $1; }
    | TYPE_ID         { $$ = type_lookup($1); }  /* tipo do usuário (#19) */
    | CARET tipo_base { $$ = type_ptr($2); }     /* ^No: ponteiro para No */
    ;

tipo_prim
    : INT   { $$ = &type_int; }
    | FLOAT { $$ = &type_float; }
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

/* tamanhos do inicializador [e; n, m]: guardados como array de void, só a forma importa */
tamanhos
    : tamanho                   { $$ = type_array(&type_void, $1); }
    | tamanhos COMMA tamanho    { $$ = type_add_dim($1, $3); }
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
            Subprog *dono = scope_level(scope_depth())->owner;
            verifica_retorno(dono, 0, atr("", &type_void, yylineno), yylineno);
            int main_proc = dono && !dono->is_func && strcmp(dono->name, "main") == 0;
            $$ = main_proc ? "return 0;\n" : "return;\n";  /* proc main vira int main */
        }
    | RETURN expr SEMI  {
            Subprog *dono = scope_level(scope_depth())->owner;
            verifica_retorno(dono, 1, $2, yylineno);
            $$ = cod_fmt("return %s;\n", (dono && dono->is_func ? coage($2, dono->ret) : $2).cod);
        }
    ;

/* declare_var depois do inicializador: "int x := x" acusa x não declarado */
decl_var
    : tipo ID ASSIGN expr SEMI  {
            verifica_atribuicao($1, $4.tipo, $4.linha, "inicialização");
            int sombra = find_name($2) != NULL;  /* esconde um nome de fora: o C precisa de temporário */
            declare_var($2, C_VAR, $1, 0, yylineno);
            $$ = gen_decl($1, $2, coage($4, $1), sombra);
        }
    | tipo ID ASSIGN LBRACKET expr SEMI tamanhos RBRACKET SEMI {
            declare_var($2, C_VAR, $1, 0, yylineno);
            int k = verifica_preenche($2, $1, $7, $5, yylineno);
            $$ = k ? gen_decl_preenche($1, $2, $5, k) : "";
        }
    | tipo ID ASSIGN LBRACE record_values RBRACE SEMI {
            declare_var($2, C_VAR, $1, 0, yylineno);
            $$ = gen_decl_registro($1, $2, $5, yylineno);
            libera_args($5);
        }
    ;

atrib
    : alvo ASSIGN expr SEMI     {
            if ($1.eh_const)
                erro_semantico($1.linha, "não é possível atribuir à constante '%s'", $1.cod);
            else
                verifica_atribuicao($1.tipo, $3.tipo, $1.linha, "atribuição");
            $$ = gen_atrib($1, coage($3, $1.tipo));
        }
    ;

alvo
    /* tipo vem da tabela de símbolos; não declarado => type_erro (erro já reportado por use_var) */
    : ID    {
            Symbol *s = use_var($1, yylineno);
            /* parâmetros ref que não são arrays são ponteiros no C */
            int ptr = s && s->category == C_PARAM && s->is_ref && s->type->kind != K_ARRAY;
            $$ = atr(ptr ? cod_fmt("(*%s)", $1) : $1, s ? s->type : &type_erro, yylineno);
            $$.eh_const = s && s->category == C_CONST;
        }
    | alvo LBRACKET expr RBRACKET { $$ = verifica_indexacao($1, $3, yylineno); }
    | alvo DOT name               { $$ = verifica_campo($1, $3, yylineno); }
    ;

/* chamada consulta só a tabela de subprogramas: x() com x variável é erro */
chamada
    : ID LPAREN RPAREN      { $$ = verifica_chamada($1, NULL, yylineno); }
    | ID LPAREN args RPAREN { $$ = verifica_chamada($1, $3, yylineno); }
    ;

args
    : arg
    | arg COMMA args  { $1->next = $3; $$ = $1; }
    ;

arg
    : expr     { $$ = arg_novo($1.cod, $1.tipo, 0, $1.linha); }
    /* texto não é expressão: só aparece como argumento; o semântico só o aceita em escrever */
    | LITERAL_TEXTO { $$ = arg_novo($1, &type_texto, 0, yylineno); }
    /* ref = passagem por referência: ponteiro no C; array já é ponteiro */
    | REF alvo {
            if ($2.eh_const)
                erro_semantico($2.linha, "constante '%s' não pode ser passada como 'ref'", $2.cod);
            char *cod = $2.tipo->kind == K_ARRAY ? $2.cod : cod_fmt("&%s", $2.cod);
            $$ = arg_novo(cod, $2.tipo, 1, $2.linha);
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
    | chamada             { $$ = verifica_chamada_expr($1); }
    | LPAREN expr RPAREN  { $$ = $2; }  /* atr_bin já pôs os parênteses */
    | MINUS fator         { $$ = atr_un("-", $2, OP_ARITMETICO); }
    | NOT fator           { $$ = atr_un("!", $2, OP_LOGICO); }
    | NULO         { $$ = atr(cod_fmt("NULL"), &type_nulo, yylineno); }
    | NOVO TYPE_ID {  /* novo No: um No zerado no heap (campos 0, ponteiros nulo) */
            TypeDesc *t = type_lookup($2);
            const char *c = c_tipo(t);
            $$ = atr(cod_fmt("((%s *)calloc(1, sizeof(%s)))", c, c), type_ptr(t), yylineno);
        }
    ;

literal
    : LITERAL_INT    { $$ = atr($1, &type_int, yylineno); }
    | LITERAL_FLOAT  { $$ = atr($1, &type_float, yylineno); }
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
