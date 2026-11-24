#include "subprog.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HashMap subprogs; // Tabela de subprogramas

Subprog *sub_insert(const char *name, Subprog data) {
    if (hm_get(&subprogs, name)) {
        return NULL;
    }

    Subprog *sub = hm_copy(&data, sizeof data);
    sub->name = strdup(name);
    hm_put(&subprogs, name, sub);

    return sub;
}

Subprog *sub_lookup(const char *name) {
    return hm_get(&subprogs, name);
}

void sub_print(void) {
    printf("Tabela de Subprogramas:\n");
    printf("%-14s %-6s %-10s %5s  %s\n", "Nome", "Categ", "Retorno", "Linha", "Parâmetros");
    printf("--------------------------------------------------------------------------------\n");

    for (Entry *entry = subprogs.first; entry; entry = entry->after) {
        Subprog *sub = entry->value;
        const char *category = sub->is_builtin ? "embut" : sub->is_func ? "func" : "proc";
        char *ret = type_str(sub->ret);

        printf("%-14s %-6s %-10s %5d  (", sub->name, category, ret, sub->line);
        free(ret);

        for (Symbol *param = sub->params; param; param = param->next_param) {
            char *type = type_str(param->type);
            printf("%s%s%s %s", param == sub->params ? "" : ", ", param->is_ref ? "ref " : "", type, param->name);
            free(type);
        }
        printf(")\n");
    }
    putchar('\n');
}
