#include "symtab.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SCOPE_DEPTH 100

static HashMap symbols; // Tabela de símbolos
static Scope stack[MAX_SCOPE_DEPTH]; // Pilha de escopos
static int depth; // Profundidade da pilha de escopos
static int block_counter; // Contador de blocos para gerar nomes únicos

/* Funções auxiliares */

/**
 * @brief Cria uma chave única para o símbolo com base no escopo e no nome.
 * @param scope Nome do escopo.
 * @param name Nome do símbolo.
 * @return Ponteiro para a chave criada (deve ser liberado pelo chamador).
 */
static char *make_key(const char *scope, const char *name) {
    size_t len = strlen(scope) + strlen(name) + 2; // +1 para o separador e +1 para o '\0'
    char *key = malloc(len);

    if (!key) {
        fprintf(stderr, "Erro de alocação de memória para chave.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(key, len, "%s#%s", scope, name);

    return key;
}


Symbol *table_lookup(const char *scope, const char *name) {
    char *key = make_key(scope, name);
    Symbol *sym = hm_get(&symbols, key);

    free(key);

    return sym;
}

Symbol *table_insert(const char *scope, const char *name, Symbol data) {
    char *key = make_key(scope, name);
    Symbol *sym = NULL;

    if (!hm_get(&symbols, key)) {
        sym = hm_copy(&data, sizeof data);
        sym->scope = strdup(scope);
        sym->name = strdup(name);
        hm_put(&symbols, key, sym);
    }

    free(key);

    return sym; // NULL se o símbolo já existe
}

void scope_push(const char *name, struct Subprog *owner, int is_loop) {
    if (depth == MAX_SCOPE_DEPTH) {
        fprintf(stderr, "Erro interno: mais de %d escopos aninhados.\n", MAX_SCOPE_DEPTH);
        exit(EXIT_FAILURE);
    }
    stack[depth++] = (Scope){ strdup(name), owner, is_loop };
}

void scope_push_block(int is_loop) {
    Scope *top = scope_level(depth);
    size_t len = strlen(top->name) + 20; // Espaço suficiente para o nome do bloco e o contador
    char *block_name = malloc(len);

    snprintf(block_name, len, "%s#block%d", top->name, block_counter++);
    scope_push(block_name, top->owner, top->is_loop || is_loop);

    free(block_name); // scope_push guardou uma cópia
}

void scope_pop(void) {
    if (depth <= 0) {
        fprintf(stderr, "Erro interno: tentativa de desempilhar escopo vazio ou inexistente.\n");
        exit(EXIT_FAILURE);
    }

    free(stack[--depth].name); // Libera o nome do escopo
}

Scope *scope_level(int level) {
    return (level > 0 && level <= depth) ? &stack[level - 1] : NULL;
}

int scope_depth(void) {
    return depth;
}

Symbol *find_name(const char *name) {
    for (int i = scope_depth(); i > 0; i--) {
        Scope *scope = scope_level(i);
        Symbol *sym = table_lookup(scope->name, name);
        if (sym) {
            return sym;
        }
    }
    return NULL; // Não encontrado
}

void table_print(void) {
    static const char *category[] = {"var", "const", "param"};

    printf("Tabela de Símbolos:\n");
    printf("%-24s %-6s %-10s %3s %5s\n", "Chave", "Categ", "Tipo", "Ref", "Linha");
    printf("--------------------------------------------------------------------------------\n");

    for (Entry *entry = symbols.first; entry; entry = entry->after) {
        Symbol *sym = entry->value;
        char *type = type_str(sym->type);

        printf("%-24s %-6s %-10s %3d %5d\n", entry->key, category[sym->category], type, sym->is_ref, sym->line);
        free(type);
    }
    putchar('\n');
}
