#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantico.h"
#include "gen/gen.h"

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

Atr coage(Atr e, const TypeDesc *destino) {
    if (destino->kind != K_FLOAT || e.tipo->kind != K_INT) return e;
    return atr(cod_fmt("((double)%s)", e.cod), &type_float, e.linha);
}

Atr atr_bin(Atr a, const char *op_c, Atr b, Op op) {
    if (a.tipo->kind == K_ERRO || b.tipo->kind == K_ERRO) {
        return atr(cod_fmt("(%s %s %s)", a.cod, op_c, b.cod), &type_erro, a.linha);
    }
    TypeDesc *res = tipo_resultado(op, a.tipo, b.tipo);
    if (res->kind == K_ERRO) {
        char *sa = type_str(a.tipo), *sb = type_str(b.tipo);
        const char *op_txt = strcmp(op_c, "==") == 0 ? "=" : op_c;
        erro_semantico(a.linha, "operador '%s' incompatível com '%s' e '%s'", op_txt, sa, sb);
        free(sa);
        free(sb);
    }
    TypeDesc *comum = &type_erro;
    if (compativel(a.tipo, b.tipo, &comum)) {  /* int op float: o int vira double no C */
        a = coage(a, comum);
        b = coage(b, comum);
    }
    return atr(cod_fmt("(%s %s %s)", a.cod, op_c, b.cod), res, a.linha);
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

Atr verifica_indexacao(Atr base, Atr idx, int linha) {
    char *cod = cod_fmt("%s[%s]", base.cod, idx.cod);
    if (idx.tipo->kind != K_ERRO && idx.tipo->kind != K_INT) {
        char *si = type_str(idx.tipo);
        erro_semantico(idx.linha, "índice de '%s' tem de ser 'int', mas recebeu '%s'", base.cod, si);
        free(si);
    }
    TypeDesc *t = base.tipo;
    if (t->kind == K_ERRO) return atr(cod, &type_erro, linha);
    if (t->kind != K_ARRAY) {
        char *st = type_str(t);
        erro_semantico(linha, "'%s' não é array (tipo '%s')", base.cod, st);
        free(st);
        return atr(cod, &type_erro, linha);
    }
    Atr res = atr(cod, t->elem, linha);
    res.eh_const = base.eh_const;
    return res;
}

Atr verifica_campo(Atr base, const char *nome, int linha) {
    if (base.tipo->kind == K_RECORD) {
        for (const Field *campo = base.tipo->fields; campo; campo = campo->next) {
            if (strcmp(campo->name, nome) == 0) {
                Atr res = atr(gen_campo(base.cod, nome), campo->type, linha);
                res.eh_const = base.eh_const;
                return res;
            }
        }
        erro_semantico(linha, "registro '%s' não tem campo '%s'", base.tipo->name, nome);
    } else if (base.tipo->kind != K_ERRO) {
        erro_semantico(linha, "acesso ao campo '%s' exige um registro", nome);
    }
    return atr(base.cod, &type_erro, linha);
}

Arg *arg_novo(char *cod, TypeDesc *tipo, int is_ref, int linha) {
    Arg *a = malloc(sizeof *a);
    if (!a) {
        perror("arg_novo");
        exit(1);
    }
    a->cod = cod;
    a->tipo = tipo;
    a->is_ref = is_ref;
    a->linha = linha;
    a->next = NULL;
    return a;
}

static char *junta_args_cod(const Arg *args) {
    char *res = "";
    for (const Arg *a = args; a; a = a->next) {
        res = *res ? cod_fmt("%s, %s", res, a->cod) : a->cod;
    }
    return res;
}

void libera_args(Arg *args) {
    while (args) {
        Arg *next = args->next;
        free(args);
        args = next;
    }
}

static int eh_escalar(const TypeDesc *t) {
    return t->kind == K_INT || t->kind == K_FLOAT || t->kind == K_BOOL;
}

static void verifica_builtin(const Subprog *p, const Arg *args, int n_args, int linha) {
    if (n_args != 1) {
        erro_semantico(linha, "'%s' espera 1 argumento, mas recebeu %d", p->name, n_args);
        return;
    }
    if (strcmp(p->name, "ler") == 0) {
        if (!args->is_ref) {
            erro_semantico(args->linha, "argumento de 'ler' exige 'ref'");
        }
        if (args->tipo->kind != K_ERRO && !eh_escalar(args->tipo)) {
            char *st = type_str(args->tipo);
            erro_semantico(args->linha, "'ler' espera tipo escalar, mas recebeu '%s'", st);
            free(st);
        }
    } else if (strcmp(p->name, "escrever") == 0) {
        if (args->is_ref) {
            erro_semantico(args->linha, "argumento de 'escrever' não aceita 'ref'");
        }
        if (args->tipo->kind != K_ERRO && args->tipo->kind != K_TEXTO && !eh_escalar(args->tipo)) {
            char *st = type_str(args->tipo);
            erro_semantico(args->linha, "'escrever' espera tipo escalar ou texto, mas recebeu '%s'", st);
            free(st);
        }
    }
}

/* Nome C do embutido: C não tem sobrecarga, então cada tipo tem a sua função no prólogo. */
static const char *nome_c_builtin(const char *nome, const Arg *args) {
    Kind k = args ? args->tipo->kind : K_INT;
    if (strcmp(nome, "ler") == 0) return k == K_FLOAT ? "ler_float" : "ler_int";  /* bool é int no C */
    switch (k) {
    case K_FLOAT: return "escrever_float";
    case K_BOOL:  return "escrever_bool";
    case K_TEXTO: return "escrever_texto";
    default:      return "escrever_int";
    }
}

Atr verifica_chamada(const char *nome, Arg *args, int linha) {
    Subprog *p = use_sub(nome, linha);
    if (!p) {
        char *cod = cod_fmt("%s(%s)", nome, junta_args_cod(args));
        libera_args(args);
        return atr(cod, &type_erro, linha);
    }

    int n_args = 0;
    for (Arg *a = args; a; a = a->next) n_args++;

    if (p->is_builtin) {
        verifica_builtin(p, args, n_args, linha);
        char *cod = cod_fmt("%s(%s)", nome_c_builtin(nome, args), junta_args_cod(args));
        libera_args(args);
        return atr(cod, p->ret, linha);
    }

    if (n_args != p->n_params) {
        erro_semantico(linha, "'%s' espera %d argumento(s), mas recebeu %d", nome, p->n_params, n_args);
    }

    Symbol *param = p->params;
    Arg *arg = args;
    for (int pos = 1; param && arg; pos++, param = param->next_param, arg = arg->next) {
        if (param->is_ref && !arg->is_ref) {
            erro_semantico(arg->linha, "argumento %d de '%s' exige 'ref'", pos, nome);
        } else if (!param->is_ref && arg->is_ref) {
            erro_semantico(arg->linha, "argumento %d de '%s' não é parâmetro 'ref'", pos, nome);
        }

        if (param->is_ref) {
            if (param->type->kind != K_ERRO && arg->tipo->kind != K_ERRO && !type_equal(param->type, arg->tipo)) {
                char *sp = type_str(param->type), *sa = type_str(arg->tipo);
                erro_semantico(arg->linha, "argumento %d ('ref') de '%s' incompatível: esperado '%s', recebeu '%s'",
                               pos, nome, sp, sa);
                free(sp);
                free(sa);
            }
        } else if (!tipo_atribuivel(param->type, arg->tipo)) {
            char *sp = type_str(param->type), *sa = type_str(arg->tipo);
            erro_semantico(arg->linha, "argumento %d de '%s' incompatível: esperado '%s', recebeu '%s'",
                           pos, nome, sp, sa);
            free(sp);
            free(sa);
        } else {  /* por valor: int em parâmetro float vira (double) no C */
            arg->cod = coage(atr(arg->cod, arg->tipo, arg->linha), param->type).cod;
        }
    }

    char *cod = cod_fmt("%s(%s)", nome, junta_args_cod(args));
    libera_args(args);
    return atr(cod, p->ret, linha);
}

Atr verifica_chamada_expr(Atr ch) {
    if (ch.tipo->kind == K_VOID) {
        erro_semantico(ch.linha, "procedimento não pode ser usado em expressão");
        return atr(ch.cod, &type_erro, ch.linha);
    }
    return ch;
}

void verifica_retorno(Subprog *sub, int tem_expr, Atr expr, int linha) {
    if (!sub) return;
    sub->has_return = 1;
    if (!tem_expr) {
        if (sub->is_func) {
            erro_semantico(linha, "função '%s' exige valor no return", sub->name);
        }
        return;
    }
    if (!sub->is_func) {
        erro_semantico(linha, "procedimento '%s' não pode retornar valor", sub->name);
        return;
    }
    verifica_atribuicao(sub->ret, expr.tipo, expr.linha, "retorno");
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
    /* Só o escopo do topo: num escopo de dentro o nome esconde o de fora (shadowing, critério 2D) */
    Symbol *sym = table_lookup(scope_level(scope_depth())->name, nome);
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

Symbol *declare_var(const char *name, Category category, TypeDesc *type, int is_ref, int line) {
    if (!nome_livre(name, line)) {
        return NULL;
    }

    Scope *top = scope_level(scope_depth());
    Symbol data = { .category = category, .type = type, .is_ref = is_ref, .line = line };

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

TypeDesc *declare_type(const char *name, Field *fields, int line) {
    if (!nome_livre(name, line)) {
        return NULL;
    }

    for (const Field *campo = fields; campo; campo = campo->next) {
        for (const Field *outro = campo->next; outro; outro = outro->next) {
            if (strcmp(campo->name, outro->name) == 0) {
                erro_semantico(line, "campo '%s' já declarado no registro '%s'", campo->name, name);
                return NULL;
            }
        }
    }

    return type_insert(name, (TypeDesc){ .kind = K_RECORD, .fields = fields });
}
