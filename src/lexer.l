%{
  #include <stdio.h>
  #include <string.h>
  #include "y.tab.h"

  extern int yyerror(char *msg);
%}

%option yylineno
%option noinput nounput

digito   [0-9]
letra    [a-zA-Z]
ident    {letra}({letra}|{digito}|_)*

%%

"//".*                  ;   /* comentário de linha: até o fim da linha, descartado */
[ \t\r\n]+               ;   /* espaços e quebras de linha: apenas separam lexemas */

"const"       { return CONST; }
"proc"        { return PROC; }
"func"        { return FUNC; }
"int"         { return INT; }
"bool"        { return BOOL; }
"if"          { return IF; }
"else"        { return ELSE; }
"while"       { return WHILE; }
"for"         { return FOR; }
"to"          { return TO; }
"switch"      { return SWITCH; }
"case"        { return CASE; }
"default"     { return DEFAULT; }
"break"       { return BREAK; }
"continue"    { return CONTINUE; }
"return"      { return RETURN; }
"out"         { return OUT; }
"true"        { return TRUE; }
"false"       { return FALSE; }

":="          { return ASSIGN; }
"="           { return EQ; }
"!="          { return NEQ; }
"<="          { return LE; }
"<"           { return LT; }
">="          { return GE; }
">"           { return GT; }
"+"           { return PLUS; }
"-"           { return MINUS; }
"*"           { return STAR; }
"/"           { return SLASH; }
"&&"          { return AND; }
"||"          { return OR; }
"!"           { return NOT; }
"->"          { return ARROW; }

"("           { return LPAREN; }
")"           { return RPAREN; }
"["           { return LBRACKET; }
"]"           { return RBRACKET; }
"{"           { return LBRACE; }
"}"           { return RBRACE; }
","           { return COMMA; }
";"           { return SEMI; }
"."           { return DOT; }

{digito}+     { yylval.str = strdup(yytext); return LITERAL_INT; }
{ident}       { yylval.str = strdup(yytext); return ID; }

.             { yyerror("caractere inválido"); }

%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif
