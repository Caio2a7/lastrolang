#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantico.h"

/* Contador de erros semânticos; define o código de saída do compilador. */
static int total_erros = 0;

static int eh_numerico(const TypeDesc *t) {
    return t->kind == K_INT || t->kind == K_FLOAT;
}

Atr atr(char *cod, TypeDesc *tipo, int linha) {
    Atr a = { cod, tipo, linha, 0 };
    return a;
}

int compativel(TypeDesc *a, TypeDesc *b, TypeDesc **res) {
    if (a->kind == K_ERRO || b->kind == K_ERRO) {
        if (res) *res = &type_erro;
        return 1;  /* propaga erro sem cascata */
    }
    if (a->kind == K_VOID || b->kind == K_VOID) {
        if (res) *res = &type_erro;
        return 0;
    }
    if (type_equal(a, b)) {
        if (res) *res = a;
        return 1;
    }
    if ((a->kind == K_INT && b->kind == K_FLOAT) || (a->kind == K_FLOAT && b->kind == K_INT)) {
        if (res) *res = &type_float;
        return 1;
    }
    if (res) *res = &type_erro;
    return 0;
}

TypeDesc *tipo_resultado(Op op, TypeDesc *a, TypeDesc *b) {
    TypeDesc *res = &type_erro;
    if (!compativel(a, b, &res) || res->kind == K_ERRO) return &type_erro;
    switch (op) {
    case OP_ARITMETICO:
        return eh_numerico(res) ? res : &type_erro;
    case OP_RELACIONAL:
        return eh_numerico(res) ? &type_bool : &type_erro;
    case OP_IGUALDADE:
        return (eh_numerico(res) || res->kind == K_BOOL) ? &type_bool : &type_erro;
    case OP_LOGICO:
        return res->kind == K_BOOL ? &type_bool : &type_erro;
    }
    return &type_erro;
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
    char *cod = cod_fmt("(%s %s %s)", a.cod, op_c, b.cod);
    if (a.tipo->kind == K_ERRO || b.tipo->kind == K_ERRO) {
        return atr(cod, &type_erro, a.linha);
    }
    TypeDesc *res = tipo_resultado(op, a.tipo, b.tipo);
    if (res->kind == K_ERRO) {
        char *sa = type_str(a.tipo), *sb = type_str(b.tipo);
        const char *op_txt = strcmp(op_c, "==") == 0 ? "=" : op_c;
        erro_semantico(a.linha, "operador '%s' incompatível com '%s' e '%s'", op_txt, sa, sb);
        free(sa);
        free(sb);
    }
    return atr(cod, res, a.linha);
}

Atr atr_un(const char *op_c, Atr a, Op op) {
    char *cod = cod_fmt("(%s%s)", op_c, a.cod);
    if (a.tipo->kind == K_ERRO) {
        return atr(cod, &type_erro, a.linha);
    }
    TypeDesc *res = tipo_resultado(op, a.tipo, a.tipo);
    if (res->kind == K_ERRO) {
        char *sa = type_str(a.tipo);
        erro_semantico(a.linha, "operador '%s' incompatível com '%s'", op_c, sa);
        free(sa);
    }
    return atr(cod, res, a.linha);
}

int tipo_atribuivel(const TypeDesc *destino, const TypeDesc *origem) {
    if (destino->kind == K_ERRO || origem->kind == K_ERRO) return 1;
    if (destino->kind == K_VOID || origem->kind == K_VOID) return 0;
    if (type_equal(destino, origem)) return 1;
    return destino->kind == K_FLOAT && origem->kind == K_INT;
}

int verifica_atribuicao(const TypeDesc *destino, const TypeDesc *origem, int linha, const char *ctx) {
    if (tipo_atribuivel(destino, origem)) return 1;
    char *sd = type_str(destino), *so = type_str(origem);
    erro_semantico(linha, "%s incompatível: esperado '%s', recebeu '%s'", ctx, sd, so);
    free(sd);
    free(so);
    return 0;
}

Atr verifica_indexacao(const char *nome, Symbol *s, Atr idx, int linha) {
    char *cod = cod_fmt("%s[%s]", nome, idx.cod);
    if (idx.tipo->kind != K_ERRO && idx.tipo->kind != K_INT) {
        char *si = type_str(idx.tipo);
        erro_semantico(idx.linha, "índice de '%s' tem de ser 'int', mas recebeu '%s'", nome, si);
        free(si);
    }
    TypeDesc *t = s ? s->type : &type_erro;
    if (t->kind == K_ERRO) return atr(cod, &type_erro, linha);
    if (t->kind != K_ARRAY) {
        char *st = type_str(t);
        erro_semantico(linha, "'%s' não é array (tipo '%s')", nome, st);
        free(st);
        return atr(cod, &type_erro, linha);
    }
    Atr res = atr(cod, t->elem, linha);
    res.eh_const = (s->category == C_CONST);
    return res;
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
