#ifndef GEN_H
#define GEN_H

/**
 * @file gen.h
 * @brief Geração do C simplificado: o controle de fluxo vira só "if (c) goto" e rótulos,
 *        sem while, for nem else. Todas as funções devolvem texto C em memória nova.
 */

#include "semantic/semantico.h"

/* Includes e os embutidos ler/escrever, emitidos antes de tudo. */
extern const char *gen_prologo;

/* Rótulos do laço mais interno: cont é o destino do continue, brk o do break. */
typedef struct { int cont, brk; } Laco;

/* Abre um laço com dois rótulos novos; chamado antes do corpo, para o break enxergá-los. */
void laco_abre(void);

/* Laço mais interno, ou NULL fora de laço. */
Laco *laco_topo(void);

void laco_fecha(void);

/* Tipo C de um tipo Lastro: bool vira int. */
const char *c_tipo(const TypeDesc *t);

/* Declaração C sem ';': "int x", "int v[10]" ou "int m[2][3]". */
char *c_decl(const TypeDesc *t, const char *nome);

/* "{\n" + cmds indentados + "}\n". */
char *gen_bloco(const char *cmds);

/* int x = e;  arrays copiam com memcpy. sombra = 1 quando x esconde um nome de fora:
 * e é calculado antes num temporário, porque em C o x novo já valeria dentro de e. */
char *gen_decl(const TypeDesc *t, const char *nome, Atr e, int sombra);

/* int[n][m] v := [e; n, m]: laços aninhados de preenchimento com goto, um por tamanho (k). */
char *gen_decl_preenche(const TypeDesc *t, const char *nome, Atr e, int k);

char *gen_atrib(Atr alvo, Atr e);

/* if/else; senao é NULL quando não há else. */
char *gen_if(Atr c, const char *entao, const char *senao);

char *gen_while(Laco l, Atr c, const char *corpo);

/* for (int var := ini to fim), inclusivo; fim é avaliado uma vez. */
char *gen_for(Laco l, const char *var, Atr ini, Atr fim, const char *corpo);

/* Cabeçalho e corpo de um subprograma; main vira int main(void). */
char *gen_sub(const Subprog *sub, const char *corpo);

#endif
