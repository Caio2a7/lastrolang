/* Variante didática da gramática, com if sem chaves. Não faz parte do compilador.
 * O yacc acusa 1 conflito shift/reduce (dangling else): depois de "IF ( ID ) cmd",
 * com ELSE à frente, dá para reduzir o if sem else ou empilhar o ELSE. O yacc
 * resolve com shift, então o else fica com o if mais próximo.
 * Uso: echo 'if (a) if (b) x; else y;' | build/dangling_else  =>  if(a){if(b){x}else{y}} */
%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int yylex(void);
void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }
%}
%union { char *str; }
%token <str> ID
%token IF ELSE
%type <str> cmd
%%
programa
    : cmd                           { puts($1); }
    ;
cmd
    : IF '(' ID ')' cmd             { asprintf(&$$, "if(%s){%s}", $3, $5); }
    | IF '(' ID ')' cmd ELSE cmd    { asprintf(&$$, "if(%s){%s}else{%s}", $3, $5, $7); }
    | ID ';'                        { $$ = $1; }
    ;
%%
int yylex(void) {
    int c;
    while (isspace(c = getchar()));
    if (c == EOF) return 0;
    if (!isalpha(c)) return c;
    char s[64];
    int n = 0;
    do { if (n < 63) s[n++] = c; } while (isalnum(c = getchar()));
    ungetc(c, stdin);
    s[n] = 0;
    if (strcmp(s, "if") == 0) return IF;
    if (strcmp(s, "else") == 0) return ELSE;
    yylval.str = strdup(s);
    return ID;
}
int main(void) { return yyparse(); }
