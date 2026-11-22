#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"

const char *gen_prologo =
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "\n"
    "/* Lastro aceita variável não lida e o for gera rótulo de continue mesmo sem continue. */\n"
    "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n"
    "#pragma GCC diagnostic ignored \"-Wunused-but-set-variable\"\n"
    "#pragma GCC diagnostic ignored \"-Wunused-label\"\n"
    "\n"
    "static inline void ler(int *x) { if (scanf(\"%d\", x) != 1) exit(1); }\n"
    "static inline void escrever(int x) { printf(\"%d\\n\", x); }\n"
    "\n";

/* Número novo e único no programa: nomeia rótulos (_L3) e temporários (_t4).
 * Nomes Lastro começam por letra, então não colidem com esses. */
static int novo(void) {
    static int n = 0;
    return ++n;
}

#define MAX_LACOS 256
static Laco lacos[MAX_LACOS];
static int n_lacos = 0;

void laco_abre(void) {
    if (n_lacos == MAX_LACOS) {
        fprintf(stderr, "mais de %d laços aninhados\n", MAX_LACOS);
        exit(1);
    }
    lacos[n_lacos].cont = novo();
    lacos[n_lacos].brk = novo();
    n_lacos++;
}

Laco *laco_topo(void) {
    return n_lacos ? &lacos[n_lacos - 1] : NULL;
}

void laco_fecha(void) {
    n_lacos--;
}

const char *c_tipo(const TypeDesc *t) {
    switch (t->kind) {
    case K_FLOAT:  return "double";
    case K_VOID:   return "void";
    case K_RECORD: return t->name;
    default:       return "int";  /* int, bool e erro */
    }
}

char *c_decl(const TypeDesc *t, const char *nome) {
    if (t->kind == K_ARRAY) {
        return cod_fmt("%s %s[%d]", c_tipo(t->elem), nome, t->size);
    }
    return cod_fmt("%s %s", c_tipo(t), nome);
}

/* Põe 4 espaços no começo de cada linha não vazia. */
static char *indenta(const char *s) {
    size_t linhas = 1;
    for (const char *p = s; *p; p++) {
        linhas += *p == '\n';
    }
    char *out = malloc(strlen(s) + 4 * linhas + 1), *o = out;
    if (!out) {
        perror("indenta");
        exit(1);
    }
    for (int inicio = 1; *s; s++) {
        if (inicio && *s != '\n') {
            memcpy(o, "    ", 4);
            o += 4;
        }
        *o++ = *s;
        inicio = *s == '\n';
    }
    *o = '\0';
    return out;
}

char *gen_bloco(const char *cmds) {
    return cod_fmt("{\n%s}\n", indenta(cmds));
}

/* Condição negada para o "if (...) goto". Todo cod que começa com '(' já está
 * inteiro entre parênteses (atr_bin, atr_un e "(*x)"), então não precisa de outro par. */
static char *nega(Atr c) {
    return cod_fmt(c.cod[0] == '(' ? "!%s" : "!(%s)", c.cod);
}

char *gen_decl(const TypeDesc *t, const char *nome, Atr e) {
    if (t->kind == K_ARRAY) {  /* C não atribui array */
        return cod_fmt("%s;\nmemcpy(%s, %s, sizeof %s);\n", c_decl(t, nome), nome, e.cod, nome);
    }
    return cod_fmt("%s = %s;\n", c_decl(t, nome), e.cod);
}

char *gen_decl_preenche(const TypeDesc *t, const char *nome, Atr e, int linha) {
    if (t->kind != K_ARRAY) {
        erro_semantico(linha, "'%s' não é array; [e; n] só inicializa array", nome);
        return "";
    }
    int valor = novo(), i = novo(), teste = novo(), fim = novo();
    /* o valor é calculado uma vez, antes do laço */
    return cod_fmt("%s;\n"
                   "%s = %s;\n"
                   "int _t%d = 0;\n"
                   "_L%d: ;\n"
                   "if (_t%d >= %d) goto _L%d;\n"
                   "%s[_t%d] = _t%d;\n"
                   "_t%d = _t%d + 1;\n"
                   "goto _L%d;\n"
                   "_L%d: ;\n",
                   c_decl(t, nome),
                   c_decl(t->elem, cod_fmt("_t%d", valor)), e.cod,
                   i,
                   teste,
                   i, t->size, fim,
                   nome, i, valor,
                   i, i,
                   teste,
                   fim);
}

char *gen_atrib(Atr alvo, Atr e) {
    if (alvo.tipo->kind == K_ARRAY) {  /* tamanho pelo tipo: alvo pode ser parâmetro out (ponteiro) */
        return cod_fmt("memcpy(%s, %s, sizeof(%s) * %d);\n",
                       alvo.cod, e.cod, c_tipo(alvo.tipo->elem), alvo.tipo->size);
    }
    return cod_fmt("%s = %s;\n", alvo.cod, e.cod);
}

char *gen_if(Atr c, const char *entao, const char *senao) {
    int fora = novo();
    if (!senao) {
        return cod_fmt("if (%s) goto _L%d;\n%s_L%d: ;\n", nega(c), fora, entao, fora);
    }
    int fim = novo();
    return cod_fmt("if (%s) goto _L%d;\n%sgoto _L%d;\n_L%d: ;\n%s_L%d: ;\n",
                   nega(c), fora, entao, fim, fora, senao, fim);
}

char *gen_while(Laco l, Atr c, const char *corpo) {
    return cod_fmt("_L%d: ;\nif (%s) goto _L%d;\n%sgoto _L%d;\n_L%d: ;\n",
                   l.cont, nega(c), l.brk, corpo, l.cont, l.brk);
}

char *gen_for(Laco l, const char *var, Atr ini, Atr fim, const char *corpo) {
    int lim = novo(), teste = novo();
    /* bloco próprio: dois for seguidos podem declarar o mesmo contador */
    return gen_bloco(cod_fmt("int %s = %s;\n"
                             "int _t%d = %s;\n"
                             "_L%d: ;\n"
                             "if (%s > _t%d) goto _L%d;\n"
                             "%s"
                             "_L%d: ;\n"
                             "%s = %s + 1;\n"
                             "goto _L%d;\n"
                             "_L%d: ;\n",
                             var, ini.cod,
                             lim, fim.cod,
                             teste,
                             var, lim, l.brk,
                             corpo,
                             l.cont,
                             var, var,
                             teste,
                             l.brk));
}

char *gen_sub(const Subprog *sub, const char *corpo) {
    int eh_main = strcmp(sub->name, "main") == 0;
    const char *ret = sub->is_func ? c_tipo(sub->ret) : eh_main ? "int" : "void";
    char *params = "", *copias = "";

    for (Symbol *p = sub->params; p; p = p->next_param) {
        char *d;
        if (p->type->kind == K_ARRAY && !p->is_out) {
            /* array por valor: o C passa ponteiro, então o corpo trabalha numa cópia local */
            char *orig = cod_fmt("_p_%s", p->name);
            d = c_decl(p->type, orig);
            copias = cod_fmt("%s%s;\nmemcpy(%s, %s, sizeof %s);\n",
                             copias, c_decl(p->type, p->name), p->name, orig, p->name);
        } else if (p->is_out && p->type->kind != K_ARRAY) {
            d = cod_fmt("%s *%s", c_tipo(p->type), p->name);  /* usado como (*x) no corpo */
        } else {
            d = c_decl(p->type, p->name);  /* escalar por valor ou array out */
        }
        params = *params ? cod_fmt("%s, %s", params, d) : d;
    }

    char *fim = eh_main && !sub->is_func ? "return 0;\n" : "";
    return cod_fmt("%s %s(%s) {\n%s}\n\n", ret, sub->name, *params ? params : "void",
                   indenta(cod_fmt("%s%s%s", copias, corpo, fim)));
}
