#ifndef SYMTAB_H
#define SYMTAB_H

/**
 * @file symtab.h
 * @brief Tabela de símbolos (variáveis, constantes e parâmetros) e pilha de escopos.
 */

#include "types.h"

/* Declaração antecipada: Subprog é definido em subprog.h, que inclui este
 * arquivo. Scope só guarda um ponteiro para ele, então isso basta e evita a
 * inclusão circular. */
struct Subprog;

typedef enum { C_VAR, C_CONST, C_PARAM } Category;

typedef struct Symbol {
    char *name, *scope;
    Category category;
    TypeDesc *type;             // Tipo (int, bool, int[10], registro...)
    int is_ref;                 // Indica se o parâmetro é por referência (ref)
    long value;                 // Constante: valor do literal (usado no tamanho de array)
    int line;
    struct Symbol *next_param;  // Parâmetro: próximo na ordem da assinatura
} Symbol;

typedef struct {
    char *name;
    struct Subprog *owner;  // Subprograma dono do escopo; NULL no global
    int is_loop;
} Scope;


/**
 * @brief Adiciona um novo escopo à pilha de escopos.
 * @param name Nome do escopo.
 * @param owner Subprograma dono do escopo (NULL no global).
 * @param is_loop Indica se o escopo é um loop.
 */
void scope_push(const char *name, struct Subprog *owner, int is_loop);

/**
 * @brief Adiciona um escopo de bloco com nome único; herda o dono do topo e
 *        continua marcado como loop se o topo já for loop.
 * @param is_loop Indica se o escopo é um loop.
 */
void scope_push_block(int is_loop);

/**
 * @brief Remove o último escopo da pilha de escopos; não altera a tabela.
 */
void scope_pop(void);

/**
 * @brief Retorna o escopo na posição especificada (1 = global, ..., n = topo da pilha).
 * @param level Nível do escopo a ser retornado.
 * @return Ponteiro para o escopo na posição especificada, ou NULL se a posição for inválida.
 */
Scope *scope_level(int level);

/**
 * @brief Retorna a profundidade da pilha de escopos.
 * @return Profundidade da pilha de escopos.
 */
int scope_depth(void);

/* Definição da estrutura de tabela de símbolos */

/**
 * @brief Insere um símbolo na tabela de símbolos (chave "escopo#nome").
 * @param scope Nome do escopo onde o símbolo será inserido.
 * @param name Nome do símbolo.
 * @param data Dados do símbolo.
 * @return Ponteiro para o símbolo inserido, ou NULL se o símbolo já existir no escopo especificado.
 */
Symbol *table_insert(const char *scope, const char *name, Symbol data);

/**
 * @brief Procura um símbolo na tabela de símbolos.
 * @param scope Nome do escopo onde o símbolo será procurado.
 * @param name Nome do símbolo.
 * @return Ponteiro para o símbolo encontrado, ou NULL se o símbolo não existir no escopo especificado.
 */
Symbol *table_lookup(const char *scope, const char *name);

/**
 * @brief Procura um símbolo pelo nome percorrendo a pilha de escopos, do topo até o global.
 * @param name Nome do símbolo.
 * @return Ponteiro para o símbolo visível mais interno, ou NULL se não estiver visível.
 */
Symbol *find_name(const char *name);

/**
 * @brief Imprime a tabela de símbolos, na ordem de declaração.
 */
void table_print(void);

#endif
