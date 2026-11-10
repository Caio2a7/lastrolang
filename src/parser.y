%{
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
int yyerror(char *s);
extern int yylineno;
extern char *yytext;
%}

%union {
    char *str;
}

%token <str> ID LITERAL_INT

%token CONST PROC FUNC INT BOOL IF ELSE WHILE FOR TO
%token SWITCH CASE DEFAULT BREAK CONTINUE RETURN OUT TRUE FALSE

%token ASSIGN EQ NEQ LT LE GT GE
%token PLUS MINUS STAR SLASH
%token AND OR NOT ARROW

%token LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token COMMA SEMI DOT

%start programa

%%

programa
    : subprogs
    | consts subprogs
    ;

consts
    : decl_const
    | decl_const consts
    ;

decl_const
    : CONST tipo_prim ID ASSIGN literal SEMI
    ;

subprogs
    : subprog
    | subprog subprogs
    ;

subprog
    : PROC assinatura bloco
    | FUNC assinatura ARROW tipo bloco
    ;

assinatura
    : ID LPAREN RPAREN
    | ID LPAREN params RPAREN
    ;

params
    : param
    | param COMMA params
    ;

param
    : tipo ID
    | OUT tipo ID
    ;

tipo
    : tipo_prim
    | tipo_prim LBRACKET tamanho RBRACKET
    ;

tipo_prim
    : INT
    | BOOL
    ;

tamanho
    : ID
    | LITERAL_INT
    ;

bloco
    : LBRACE RBRACE
    | LBRACE cmds RBRACE
    ;

cmds
    : cmd
    | cmd cmds
    ;

cmd
    : decl_var
    | atrib
    | chamada SEMI
    | if_stmt
    | while_stmt
    | for_stmt
    | BREAK SEMI
    | CONTINUE SEMI
    | RETURN SEMI
    | RETURN expr SEMI
    ;

decl_var
    : tipo ID ASSIGN inicial SEMI
    ;

inicial
    : expr
    | LBRACKET expr SEMI tamanho RBRACKET
    ;

atrib
    : alvo ASSIGN expr SEMI
    ;

alvo
    : ID
    | ID LBRACKET expr RBRACKET
    ;

chamada
    : ID LPAREN RPAREN
    | ID LPAREN args RPAREN
    ;

args
    : arg
    | arg COMMA args
    ;

arg
    : expr
    | OUT alvo
    ;

if_stmt
    : IF LPAREN expr RPAREN bloco
    | IF LPAREN expr RPAREN bloco ELSE if_stmt
    | IF LPAREN expr RPAREN bloco ELSE bloco
    ;

while_stmt
    : WHILE LPAREN expr RPAREN bloco
    ;

for_stmt
    : FOR LPAREN INT ID ASSIGN expr TO expr RPAREN bloco
    ;

expr
    : expr OR conj
    | conj
    ;

conj
    : conj AND rel
    | rel
    ;

rel
    : soma op_rel soma
    | soma
    ;

op_rel
    : EQ
    | NEQ
    | LT
    | LE
    | GT
    | GE
    ;

soma
    : soma PLUS termo
    | soma MINUS termo
    | termo
    ;

termo
    : termo STAR fator
    | termo SLASH fator
    | fator
    ;

fator
    : literal
    | alvo
    | chamada
    | LPAREN expr RPAREN
    | MINUS fator
    | NOT fator
    ;

literal
    : LITERAL_INT
    | TRUE
    | FALSE
    ;

%%

int main(void) {
    int status = yyparse();
    if (status == 0) {
        printf("ACEITO: programa sintaticamente correto\n");
    } else {
        printf("REJEITADO: programa contém erro\n");
    }
    return status;
}

int yyerror(char *msg) {
    fprintf(stderr, "linha %d: %s perto de '%s'\n", yylineno, msg, yytext);
    return 0;
}
