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
    "#pragma GCC diagnostic ignored \"-Wunused-const-variable\"\n"
    "\n"
    "/* Embutidos: um por tipo, porque C não tem sobrecarga. float é double no C. */\n"
    "static inline void ler_int(int *x) { if (scanf(\"%d\", x) != 1) exit(1); }\n"
    "static inline void ler_float(double *x) { if (scanf(\"%lf\", x) != 1) exit(1); }\n"
    "static inline void escrever_int(int x) { printf(\"%d\\n\", x); }\n"
    "static inline void escrever_float(double x) { printf(\"%g\\n\", x); }\n"
    "static inline void escrever_bool(int x) { puts(x ? \"true\" : \"false\"); }\n"
    "static inline void escrever_texto(const char *s) { puts(s); }\n"
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
    case K_RECORD: return cod_fmt("_tipo_%s", t->name);
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

static char *nega(Atr c) {
    return cod_fmt("!(%s)", c.cod);
}

char *gen_decl(const TypeDesc *t, const char *nome, Atr e, int sombra) {
    if (sombra) {
        /* "int x := x + 1" num escopo de dentro: em Lastro o x da direita é o de fora, mas em C
         * o x novo já vale no próprio inicializador. O valor passa antes por um temporário. */
        char *tmp = cod_fmt("_t%d", novo());
        return cod_fmt("%s%s", gen_decl(t, tmp, e, 0), gen_decl(t, nome, atr(tmp, e.tipo, e.linha), 0));
    }
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
    /* o valor é calculado uma vez, antes do laço e antes de declarar o array:
     * em "int[3] v := [v[0]; 3]" num escopo de dentro, o v[0] é o v de fora */
    return cod_fmt("%s = %s;\n"
                   "%s;\n"
                   "int _t%d = 0;\n"
                   "_L%d: ;\n"
                   "if (_t%d >= %d) goto _L%d;\n"
                   "%s[_t%d] = _t%d;\n"
                   "_t%d = _t%d + 1;\n"
                   "goto _L%d;\n"
                   "_L%d: ;\n",
                   c_decl(t->elem, cod_fmt("_t%d", valor)), e.cod,
                   c_decl(t, nome),
                   i,
                   teste,
                   i, t->size, fim,
                   nome, i, valor,
                   i, i,
                   teste,
                   fim);
}

char *gen_decl_registro(TypeDesc *t, const char *nome, const Arg *valores, int linha) {
    if (t->kind != K_RECORD) {
        erro_semantico(linha, "'%s' não é registro; { ... } só inicializa registro", nome);
        return "";
    }

    /* O temporário permite copiar campos array e mantém a nova variável fora do inicializador. */
    char *tmp = cod_fmt("_t%d", novo());
    char *decl = c_decl(t, tmp);
    char *cod = cod_fmt("%s;\n", decl);
    free(decl);

    const Field *campo = t->fields;
    for (; campo && valores; campo = campo->next, valores = valores->next) {
        char *ctx = cod_fmt("inicialização do campo '%s'", campo->name);
        if (verifica_atribuicao(campo->type, valores->tipo, valores->linha, ctx)) {
            Atr alvo = atr(gen_campo(tmp, campo->name), campo->type, valores->linha);
            Atr valor = atr(valores->cod, valores->tipo, valores->linha);
            Atr inicializador = coage(valor, campo->type);
            char *atrib = gen_atrib(alvo, inicializador);
            char *anterior = cod;
            cod = cod_fmt("%s%s", anterior, atrib);
            free(anterior);
            free(atrib);
            free(alvo.cod);
            if (inicializador.cod != valor.cod) free(inicializador.cod);
        }
        free(ctx);
    }
    char *res = "";
    if (campo || valores) {
        erro_semantico(linha, "quantidade de valores incompatível na inicialização de '%s'", nome);
    } else {
        decl = gen_decl(t, nome, atr(tmp, t, linha), 0);
        res = cod_fmt("%s%s", cod, decl);
        free(decl);
    }
    free(cod);
    free(tmp);
    return res;
}

char *gen_atrib(Atr alvo, Atr e) {
    if (alvo.tipo->kind == K_ARRAY) {  /* tamanho pelo tipo: alvo pode ser parâmetro ref (ponteiro) */
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
    int ini_t = novo(), lim = novo(), teste = novo();
    /* bloco próprio: dois for seguidos podem declarar o mesmo contador.
     * Início e limite vão para temporários antes de declarar o contador: em
     * "for (int i := i to n)" num escopo de dentro, o i da direita é o de fora. */
    return gen_bloco(cod_fmt("int _t%d = %s;\n"
                             "int _t%d = %s;\n"
                             "int %s = _t%d;\n"
                             "_L%d: ;\n"
                             "if (%s > _t%d) goto _L%d;\n"
                             "%s"
                             "_L%d: ;\n"
                             "%s = %s + 1;\n"
                             "goto _L%d;\n"
                             "_L%d: ;\n",
                             ini_t, ini.cod,
                             lim, fim.cod,
                             var, ini_t,
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
        if (p->type->kind == K_ARRAY && !p->is_ref) {
            /* array por valor: o C passa ponteiro, então o corpo trabalha numa cópia local */
            char *orig = cod_fmt("_p_%s", p->name);
            d = c_decl(p->type, orig);
            copias = cod_fmt("%s%s;\nmemcpy(%s, %s, sizeof %s);\n",
                             copias, c_decl(p->type, p->name), p->name, orig, p->name);
        } else if (p->is_ref && p->type->kind != K_ARRAY) {
            d = cod_fmt("%s *%s", c_tipo(p->type), p->name);  /* usado como (*x) no corpo */
        } else {
            d = c_decl(p->type, p->name);  /* escalar por valor ou array ref */
        }
        params = *params ? cod_fmt("%s, %s", params, d) : d;
    }

    char *fim = eh_main && !sub->is_func ? "return 0;\n" : "";
    return cod_fmt("%s %s(%s) {\n%s}\n\n", ret, sub->name, *params ? params : "void",
                   indenta(cod_fmt("%s%s%s", copias, corpo, fim)));
}

#define PREFIXO_CAMPO "_campo_"

char *gen_campo(const char *base, const char *nome) {
    return cod_fmt("(%s." PREFIXO_CAMPO "%s)", base, nome);
}

char *gen_decl_type(const TypeDesc *t) {
    /* Prefixos evitam colisões dos nomes Lastro com palavras reservadas e macros de C. */
    char *campos = cod_fmt("");
    for (const Field *campo = t->fields; campo; campo = campo->next) {
        char *nome = cod_fmt(PREFIXO_CAMPO "%s", campo->name);
        char *decl = c_decl(campo->type, nome);
        char *anterior = campos;
        campos = cod_fmt("%s%s;\n", anterior, decl);
        free(anterior);
        free(decl);
        free(nome);
    }
    char *corpo = indenta(campos);
    char *res = cod_fmt("typedef struct {\n%s} %s;\n", corpo, c_tipo(t));
    free(corpo);
    free(campos);
    return res;
}
