#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "semantico.h"

/* Contador de erros semânticos; define o código de saída do compilador. */
static int total_erros = 0;

Atr atr(char *cod, TypeDesc *tipo, int linha) {
    Atr a = { cod, tipo, linha };
    return a;
}

TypeDesc *tipo_resultado(Op op, TypeDesc *a, TypeDesc *b) {
    if (a->kind == K_ERRO || b->kind == K_ERRO) return &type_erro;  /* sem cascata de erros */
    if (op == OP_ARITMETICO) return (a->kind == K_FLOAT || b->kind == K_FLOAT) ? &type_float : &type_int;
    return &type_bool;  /* relacional e lógico */
}

/* Formata fmt/ap em memória nova; encerra o programa se faltar memória. */
static char *vcod_fmt(const char *fmt, va_list ap) {
    va_list copia;
    va_copy(copia, ap);
    int n = vsnprintf(NULL, 0, fmt, copia);  /* 1ª passada: mede */
    va_end(copia);
    char *buf = n < 0 ? NULL : malloc((size_t)n + 1);
    if (buf == NULL) {
        perror("cod_fmt");
        exit(1);
    }
    vsnprintf(buf, (size_t)n + 1, fmt, ap);  /* 2ª passada: escreve */
    return buf;
}

char *cod_fmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *s = vcod_fmt(fmt, ap);
    va_end(ap);
    return s;
}

Atr atr_bin(Atr a, const char *op_c, Atr b, Op op) {
    return atr(cod_fmt("(%s %s %s)", a.cod, op_c, b.cod),
               tipo_resultado(op, a.tipo, b.tipo), a.linha);
}

Atr atr_un(const char *op_c, Atr a, Op op) {
    return atr(cod_fmt("(%s%s)", op_c, a.cod), tipo_resultado(op, a.tipo, a.tipo), a.linha);
}

void erro_semantico(int linha, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "linha %d: erro semântico: ", linha);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    total_erros++;
}

int erros_semanticos(void) {
    return total_erros;
}

int nome_livre(const char *nome, int linha) {
    Symbol *sym = find_name(nome);  /* escopo atual e todos os envolventes */
    if (sym) {
        erro_semantico(linha, "'%s' já declarado no escopo '%s'", nome, sym->scope);
        return 0;
    }
    if (sub_lookup(nome)) {
        erro_semantico(linha, "'%s' já é o nome de um subprograma", nome);
        return 0;
    }
    if (type_lookup(nome)) {
        erro_semantico(linha, "'%s' já é o nome de um tipo", nome);
        return 0;
    }
    return 1;
}

Symbol *declare_var(const char *name, Category category, TypeDesc *type, int is_out, int line) {
    if (!nome_livre(name, line)) {
        return NULL;
    }

    Scope *top = scope_level(scope_depth());
    Symbol data = { .category = category, .type = type, .is_out = is_out, .line = line };

    return table_insert(top->name, name, data);  /* não falha: nome_livre já olhou o topo */
}

Symbol *use_var(const char *name, int line) {
    Symbol *sym = find_name(name);

    if (!sym) {
        if (sub_lookup(name)) {
            erro_semantico(line, "'%s' é um subprograma, não uma variável", name);
        } else {
            erro_semantico(line, "'%s' não declarado", name);
        }
    }
    return sym;
}

Subprog *declare_sub(const char *name, int line) {
    if (!nome_livre(name, line)) {
        return NULL;
    }

    return sub_insert(name, (Subprog){ .ret = &type_void, .line = line });
}

Subprog *use_sub(const char *name, int line) {
    Subprog *sub = sub_lookup(name);

    if (!sub) {
        if (find_name(name)) {
            erro_semantico(line, "'%s' não é um subprograma", name);
        } else {
            erro_semantico(line, "subprograma '%s' não declarado", name);
        }
    }
    return sub;
}

void append_param(Subprog *sub, Symbol *param) {
    if (!sub || !param) {
        return;
    }

    Symbol **next_param_ptr = &sub->params;

    while (*next_param_ptr) {
        next_param_ptr = &(*next_param_ptr)->next_param;
    }

    *next_param_ptr = param;
    sub->n_params++;
}
