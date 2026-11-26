#include "types.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TypeDesc type_int   = { .kind = K_INT,   .name = "int" };
TypeDesc type_float = { .kind = K_FLOAT, .name = "float" };
TypeDesc type_bool  = { .kind = K_BOOL,  .name = "bool" };
TypeDesc type_void  = { .kind = K_VOID,  .name = "void" };
TypeDesc type_erro  = { .kind = K_ERRO,  .name = "erro" };
TypeDesc type_texto = { .kind = K_TEXTO, .name = "texto" };
TypeDesc type_nulo  = { .kind = K_NULO,  .name = "nulo" };

static HashMap types; // Tabela de tipos

void type_init(void) {
    hm_put(&types, "int", &type_int);
    hm_put(&types, "float", &type_float);
    hm_put(&types, "bool", &type_bool);
}

TypeDesc *type_insert(const char *name, TypeDesc data) {
    if (hm_get(&types, name)) {
        return NULL;
    }

    TypeDesc *type = hm_copy(&data, sizeof data);
    type->name = strdup(name);
    hm_put(&types, name, type);

    return type;
}

TypeDesc *type_lookup(const char *name) {
    return hm_get(&types, name);
}

TypeDesc *type_array(TypeDesc *elem, int size) {
    return hm_copy(&(TypeDesc){ .kind = K_ARRAY, .elem = elem, .size = size }, sizeof(TypeDesc));
}

TypeDesc *type_add_dim(TypeDesc *t, int size) {
    if (t->kind != K_ARRAY) {
        return type_array(t, size);
    }
    return type_array(type_add_dim(t->elem, size), t->size);
}

TypeDesc *type_ptr(TypeDesc *elem) {
    return hm_copy(&(TypeDesc){ .kind = K_PTR, .elem = elem }, sizeof(TypeDesc));
}

int type_equal(const TypeDesc *a, const TypeDesc *b) {
    if (a == b) {
        return 1;
    }

    if (a->kind == K_ARRAY && b->kind == K_ARRAY) {
        return a->size == b->size && type_equal(a->elem, b->elem);
    }

    if (a->kind == K_PTR && b->kind == K_PTR) {
        return type_equal(a->elem, b->elem);
    }

    return 0;
}

char *type_str(const TypeDesc *t) {
    if (t->kind == K_PTR) {  /* ponteiro não tem nome: monta "^No" */
        char *elem = type_str(t->elem);
        size_t len = strlen(elem) + 2;  /* '^' + '\0' */
        char *str = malloc(len);
        snprintf(str, len, "^%s", elem);
        free(elem);
        return str;
    }

    if (t->kind != K_ARRAY) {
        return strdup(t->name);
    }

    /* a dimensão de fora vem antes das do elemento: int[2] de int[3] é "int[2][3]" */
    char *elem = type_str(t->elem);
    size_t len = strlen(elem) + 16;
    char *str = malloc(len);
    int base = (int)strcspn(elem, "[");

    snprintf(str, len, "%.*s[%d]%s", base, elem, t->size, elem + base);
    free(elem);

    return str;
}

void type_print(void) {
    printf("Tabela de Tipos:\n");
    printf("%-14s %-10s %s\n", "Nome", "Classe", "Campos");
    printf("--------------------------------------------------------------------------------\n");

    for (Entry *entry = types.first; entry; entry = entry->after) {
        TypeDesc *type = entry->value;
        printf("%-14s %-10s", entry->key, type->kind == K_RECORD ? "registro" : "primitivo");

        for (Field *field = type->fields; field; field = field->next) {
            char *field_type = type_str(field->type);
            printf(" %s %s;", field_type, field->name);
            free(field_type);
        }
        putchar('\n');
    }
    putchar('\n');
}
