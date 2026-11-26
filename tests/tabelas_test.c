/* Teste das issues #5 e #6: tabelas de tipos, subprogramas e símbolos, e pilha de escopos.
 * Rodar com: make test-tabelas */
#include "semantic/semantico.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    Symbol var_int = { .category = C_VAR, .type = &type_int };

    /* Tipos: primitivos registrados, registro nominal, array estrutural */
    type_init();
    assert(type_lookup("int") == &type_int);
    assert(type_lookup("Conta") == NULL);
    TypeDesc *conta = type_insert("Conta", (TypeDesc){ .kind = K_RECORD });
    assert(conta && type_lookup("Conta") == conta);
    assert(type_insert("Conta", (TypeDesc){ .kind = K_RECORD }) == NULL);
    assert(type_equal(type_array(&type_int, 10), type_array(&type_int, 10)));
    assert(!type_equal(type_array(&type_int, 10), type_array(&type_int, 5)));
    assert(!type_equal(type_array(&type_int, 10), type_array(&type_bool, 10)));
    char *s = type_str(type_array(&type_int, 10));
    assert(strcmp(s, "int[10]") == 0);
    free(s);
    s = type_str(type_add_dim(type_array(&type_int, 2), 3));
    assert(strcmp(s, "int[2][3]") == 0);
    free(s);

    /* Símbolos: insere, busca, duplicata */
    assert(table_insert("global", "TAXA", (Symbol){ .category = C_CONST, .type = &type_int }));
    assert(table_lookup("global", "TAXA")->category == C_CONST);
    assert(table_insert("global", "TAXA", var_int) == NULL);
    assert(table_lookup("global", "nada") == NULL);

    /* ab#c e a#bc são chaves diferentes */
    assert(table_insert("ab", "c", var_int) && table_insert("a", "bc", var_int));
    assert(table_lookup("ab", "c") != table_lookup("a", "bc"));

    /* Colisão: main#limite e main#v59 caem no mesmo bucket (689) e as duas são achadas */
    Symbol *limite = table_insert("main", "limite", var_int);
    Symbol *v59 = table_insert("main", "v59", var_int);
    assert(limite && v59 && limite != v59);
    assert(table_lookup("main", "limite") == limite && table_lookup("main", "v59") == v59);

    /* Subprogramas: inserção, duplicata e lista de parâmetros em ordem */
    scope_push("global", NULL, 0);
    Subprog *f = declare_sub("f", 1);
    assert(f && sub_lookup("f") == f && f->ret == &type_void);
    assert(sub_insert("f", (Subprog){ 0 }) == NULL);
    scope_push("f", f, 0);
    append_param(f, declare_var("a", C_PARAM, &type_int, 0, 1));
    append_param(f, declare_var("b", C_PARAM, &type_bool, 1, 1));
    assert(f->n_params == 2);
    assert(strcmp(f->params->name, "a") == 0 && strcmp(f->params->next_param->name, "b") == 0);
    assert(f->params->next_param->is_ref == 1);

    /* Pilha: o bloco herda o dono e o is_loop */
    scope_push_block(1);
    assert(scope_level(scope_depth())->owner == f);
    scope_push_block(0);
    assert(scope_level(scope_depth())->is_loop == 1);
    scope_pop();
    scope_pop();
    scope_pop();

    /* Escopo aninhado acha o mais interno; após pop, some de vista mas continua na tabela */
    Subprog *m = declare_sub("main2", 2);
    scope_push("main2", m, 0);
    Symbol *x = declare_var("x", C_VAR, &type_int, 0, 3);
    scope_push_block(0);
    char bloco[64];
    strcpy(bloco, scope_level(scope_depth())->name);
    Symbol *y = declare_var("y", C_VAR, &type_int, 0, 4);
    assert(find_name("x") == x && find_name("y") == y);
    scope_pop();
    assert(find_name("y") == NULL && table_lookup(bloco, "y") == y);

    /* Escopos irmãos não se enxergam e podem repetir o nome */
    scope_push_block(0);
    assert(find_name("y") == NULL);
    assert(declare_var("y", C_VAR, &type_int, 0, 5) != NULL);
    scope_pop();

    /* nome_livre: cada caso proibido reporta um erro e devolve 0 */
    int erros = erros_semanticos();
    assert(declare_var("x", C_VAR, &type_int, 0, 6) == NULL);         /* mesmo escopo */
    assert(declare_var("f", C_VAR, &type_int, 0, 8) == NULL);         /* nome de subprograma */
    assert(declare_var("Conta", C_VAR, &type_int, 0, 9) == NULL);     /* nome de tipo */
    assert(declare_var("livre", C_VAR, &type_int, 0, 10) != NULL);
    assert(erros_semanticos() == erros + 3);

    /* Shadowing (#49): o x de dentro esconde o de fora até o bloco fechar */
    scope_push_block(0);
    Symbol *x_dentro = declare_var("x", C_VAR, &type_bool, 0, 7);
    assert(x_dentro && x_dentro != x && find_name("x") == x_dentro);
    scope_pop();
    assert(find_name("x") == x && erros_semanticos() == erros + 3);

    scope_pop();
    scope_pop();
    assert(scope_depth() == 0);

    puts("tabelas: OK");
    return 0;
}
