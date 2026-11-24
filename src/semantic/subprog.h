#ifndef SUBPROG_H
#define SUBPROG_H

/**
 * @file subprog.h
 * @brief Tabela de subprogramas (chave = nome; subprogramas são sempre globais).
 */

#include "symtab.h"
#include "types.h"

typedef struct Subprog {
    char *name;
    int is_func;      // 0 = proc, 1 = func
    TypeDesc *ret;    // Tipo de retorno; proc usa &type_void
    Symbol *params;   // Primeiro parâmetro; os demais seguem por next_param, na ordem da assinatura
    int n_params;     // Quantidade de parâmetros
    int is_builtin;   // ler/escrever: verificação própria na #13
    int has_return;   // 1 se o corpo contém ao menos um comando return
    int line;         // Linha da declaração; 0 nos embutidos
} Subprog;

/**
 * @brief Insere um subprograma na tabela.
 * @return Ponteiro para o subprograma inserido, ou NULL se o nome já existir.
 */
Subprog *sub_insert(const char *name, Subprog data);

/**
 * @brief Procura um subprograma pelo nome.
 * @return Ponteiro para o subprograma, ou NULL se não existir.
 */
Subprog *sub_lookup(const char *name);

/**
 * @brief Imprime a tabela de subprogramas, com a assinatura de cada um.
 */
void sub_print(void);

#endif
