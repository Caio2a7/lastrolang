%{
  #include <stdio.h>
  #include <string.h>
  #include "semantic/semantico.h"
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
"float"       { return FLOAT; }
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
"ref"         { return REF; }
"true"        { return TRUE; }
"false"       { return FALSE; }
"type"        { return TYPE; }
"nulo"        { return NULO; }
"novo"        { return NOVO; }
"^"           { return CARET; }

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

  /* Texto só entre aspas na mesma linha, sem '\': o C recebe o lexema como está.
     Aspas sem fechar (ou com '\') casam só a segunda regra, que é mais curta. */
\"[^\"\\\n]*\"         { yylval.str = strdup(yytext); return LITERAL_TEXTO; }
\"[^\"\\\n]*           { yyerror("texto sem aspas de fechamento (ou com '\\')"); return ERRO; }
  /* "2.5" casa as duas regras; o casamento mais longo escolhe LITERAL_FLOAT */
{digito}+"."{digito}+  { yylval.str = strdup(yytext); return LITERAL_FLOAT; }
{digito}+     { yylval.str = strdup(yytext); return LITERAL_INT; }
  /* Nome que já é tipo vira TYPE_ID; sem isso, "Conta[10] c" e "v[i] := ..."
     ficariam ambíguos para o parser. As palavras reservadas casam antes desta regra. */
{ident}       { yylval.str = strdup(yytext); return type_lookup(yytext) ? TYPE_ID : ID; }

.             { yyerror("caractere inválido"); return ERRO; }

%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif
