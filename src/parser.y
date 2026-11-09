%{
#include <stdio.h>

int yylex(void);
int yyerror(char *s);
extern int yylineno;
extern char *yytext;
%}

%union {
    char *sValue;
}

%token <sValue> ID
%token PLUS LPAREN RPAREN ERRO

%start S

%type <sValue> E

%%
/* S -> E $   (regra aumentada, implícita pelo yacc) */
S : E
    { /* aceitação real só é relatada em main(), depois que yyparse()
         confirma que não sobrou entrada — ver comentário abaixo */ }
  ;

/* r1: E -> id */
/* r2: E -> id ( E ) */
/* r3: E -> E + id */
E : ID
    { $$ = $1;
      printf("reduz r1: E -> id        [%s]\n", $1); }
  | ID LPAREN E RPAREN
    { printf("reduz r2: E -> id ( E )  [%s ( %s )]\n", $1, $3);
      $$ = $1; }
  | E PLUS ID
    { printf("reduz r3: E -> E + id    [%s + %s]\n", $1, $3);
      $$ = $1; }
  ;
%%

int main(void) {
    int status = yyparse();
    if (status == 0) {
        printf("ACEITO: entrada reconhecida pela gramática\n");
    } else {
        printf("REJEITADO: entrada não pertence à linguagem\n");
    }
    return status;
}

int yyerror(char *msg) {
    fprintf(stderr, "%d: %s perto de '%s'\n", yylineno, msg, yytext);
    return 0;
}
