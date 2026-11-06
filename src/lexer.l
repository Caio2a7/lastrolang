%{
  #include <stdio.h>
  #include <string.h>
  #include "y.tab.h"

  extern int yyerror(char *msg);
%}

%option yylineno
%option noinput nounput

%%

[ \t\n]               ;
"+"                    { return PLUS; }
"("                    { return LPAREN; }
")"                    { return RPAREN; }
[a-zA-Z][a-zA-Z0-9]*   { yylval.sValue = strdup(yytext);
                          return ID; }
.                      { yyerror("caractere inválido"); }

%%

#ifndef yywrap
   int yywrap(void) { return 1; }
#endif
