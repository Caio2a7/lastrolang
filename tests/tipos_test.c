/* Teste das issues #9 e #10: compatibilidade de expressões, atribuição e indexação.
 * Rodar com: make test-tipos */
#include "semantic/semantico.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    TypeDesc *res = NULL;
    TypeDesc *arr = type_array(&type_int, 3);
    TypeDesc *arr5 = type_array(&type_int, 5);
    TypeDesc *arr_bool = type_array(&type_bool, 3);

    /* compativel: mesmos tipos, coerção int <-> float, void rejeitado, erro sem cascata */
    assert(compativel(&type_int, &type_int, &res) && res == &type_int);
    assert(compativel(&type_int, &type_float, &res) && res == &type_float);
    assert(compativel(&type_float, &type_int, &res) && res == &type_float);
    assert(compativel(&type_bool, &type_bool, &res) && res == &type_bool);
    assert(!compativel(&type_int, &type_bool, &res) && res == &type_erro);
    assert(!compativel(&type_void, &type_void, &res) && res == &type_erro);
    assert(compativel(&type_erro, &type_bool, &res) && res == &type_erro);

    /* Aritmético: numéricos, int op float -> float */
    assert(tipo_resultado(OP_ARITMETICO, &type_int, &type_int) == &type_int);
    assert(tipo_resultado(OP_ARITMETICO, &type_int, &type_float) == &type_float);
    assert(tipo_resultado(OP_ARITMETICO, &type_float, &type_int) == &type_float);
    assert(tipo_resultado(OP_ARITMETICO, &type_int, &type_bool) == &type_erro);
    assert(tipo_resultado(OP_ARITMETICO, &type_bool, &type_bool) == &type_erro);
    assert(tipo_resultado(OP_ARITMETICO, arr, arr) == &type_erro);

    /* Relacional (< <= > >=): numéricos -> bool */
    assert(tipo_resultado(OP_RELACIONAL, &type_int, &type_int) == &type_bool);
    assert(tipo_resultado(OP_RELACIONAL, &type_int, &type_float) == &type_bool);
    assert(tipo_resultado(OP_RELACIONAL, &type_bool, &type_bool) == &type_erro);

    /* Igualdade (= !=): tipos compatíveis (escalares) -> bool */
    assert(tipo_resultado(OP_IGUALDADE, &type_int, &type_int) == &type_bool);
    assert(tipo_resultado(OP_IGUALDADE, &type_int, &type_float) == &type_bool);
    assert(tipo_resultado(OP_IGUALDADE, &type_bool, &type_bool) == &type_bool);
    assert(tipo_resultado(OP_IGUALDADE, &type_int, &type_bool) == &type_erro);
    assert(tipo_resultado(OP_IGUALDADE, arr, arr) == &type_erro);

    /* Lógico (&& || !): só bool */
    assert(tipo_resultado(OP_LOGICO, &type_bool, &type_bool) == &type_bool);
    assert(tipo_resultado(OP_LOGICO, &type_bool, &type_int) == &type_erro);
    assert(tipo_resultado(OP_LOGICO, &type_int, &type_int) == &type_erro);

    /* atr_bin / atr_un: 1 < 2 && true válido; erros reportados sem cascata */
    int erros = erros_semanticos();
    Atr ok = atr_bin(atr_bin(atr("1", &type_int, 1), "<", atr("2", &type_int, 1), OP_RELACIONAL),
                     "&&", atr("1", &type_bool, 1), OP_LOGICO);
    assert(ok.tipo == &type_bool && erros_semanticos() == erros);

    Atr soma_err = atr_bin(atr("1", &type_int, 2), "+", atr("1", &type_bool, 2), OP_ARITMETICO);
    assert(soma_err.tipo == &type_erro && erros_semanticos() == erros + 1);
    /* propagação de type_erro: não emite novo erro */
    Atr cascata = atr_bin(soma_err, "<", atr("2", &type_int, 2), OP_RELACIONAL);
    assert(cascata.tipo == &type_erro && erros_semanticos() == erros + 1);

    Atr not_err = atr_un("!", atr("3", &type_int, 3), OP_LOGICO);
    assert(not_err.tipo == &type_erro && erros_semanticos() == erros + 2);
    Atr neg_err = atr_un("-", atr("1", &type_bool, 4), OP_ARITMETICO);
    assert(neg_err.tipo == &type_erro && erros_semanticos() == erros + 3);

    /* Atribuição (#10): iguais, int -> float permitido, float -> int e arrays distintos rejeitados */
    assert(tipo_atribuivel(&type_int, &type_int));
    assert(tipo_atribuivel(&type_float, &type_int));
    assert(!tipo_atribuivel(&type_int, &type_float));
    assert(!tipo_atribuivel(&type_int, &type_bool));
    assert(!tipo_atribuivel(&type_int, &type_void));
    assert(tipo_atribuivel(arr, type_array(&type_int, 3)));
    assert(!tipo_atribuivel(arr, arr5));
    assert(!tipo_atribuivel(arr, arr_bool));
    assert(tipo_atribuivel(&type_erro, &type_bool));
    assert(tipo_atribuivel(&type_int, &type_erro));

    /* Indexação (#10): array + int OK; não-array e índice bool geram erro e devolvem type_erro */
    Atr v = atr("v", arr, 10);
    Atr x = atr("x", &type_int, 12);
    int e10 = erros_semanticos();
    Atr elem = verifica_indexacao(v, atr("0", &type_int, 10), 10);
    assert(elem.tipo == &type_int && erros_semanticos() == e10);
    Atr idx_err = verifica_indexacao(v, atr("1", &type_bool, 11), 11);
    assert(idx_err.tipo == &type_int && erros_semanticos() == e10 + 1);
    Atr nao_arr = verifica_indexacao(x, atr("0", &type_int, 12), 12);
    assert(nao_arr.tipo == &type_erro && erros_semanticos() == e10 + 2);

    /* Matrizes (#17): int[2][3] é array de array; m[i] é int[3], m[i][j] é int */
    TypeDesc *mat = type_add_dim(type_array(&type_int, 2), 3);
    assert(mat->size == 2 && mat->elem->size == 3 && mat->elem->elem == &type_int);
    Atr linha = verifica_indexacao(atr("m", mat, 13), atr("0", &type_int, 13), 13);
    assert(type_equal(linha.tipo, type_array(&type_int, 3)));
    assert(verifica_indexacao(linha, atr("1", &type_int, 13), 13).tipo == &type_int);
    assert(erros_semanticos() == e10 + 2);
    TypeDesc *forma = type_add_dim(type_array(&type_void, 2), 3);
    assert(verifica_preenche("m", mat, forma, atr("0", &type_int, 14), 14) == 2);
    assert(verifica_preenche("m", mat, type_array(&type_void, 2), atr("0", &type_int, 14), 14) == 1);
    assert(erros_semanticos() == e10 + 3);  /* sobra int[3] para um int */
    assert(verifica_preenche("m", mat, type_add_dim(type_array(&type_void, 2), 2), atr("0", &type_int, 15), 15) == 2);
    assert(erros_semanticos() == e10 + 4);  /* 2ª dimensão: 3 != 2 */

    /* Ponteiros: ^No compara com = e != (e com nulo), recebe nulo, e p.campo desreferencia sozinho */
    TypeDesc *no = type_insert("No_ptr", (TypeDesc){ .kind = K_RECORD });
    no->fields = &(Field){ .name = "chave", .type = &type_int };
    TypeDesc *pno = type_ptr(no);
    assert(type_equal(pno, type_ptr(no)) && !type_equal(pno, type_ptr(&type_int)));
    char *spno = type_str(pno);
    assert(strcmp(spno, "^No_ptr") == 0);
    free(spno);
    assert(compativel(pno, &type_nulo, &res) && res == pno);
    assert(compativel(&type_nulo, pno, &res) && res == pno);
    assert(!compativel(pno, type_ptr(&type_int), &res));
    assert(tipo_resultado(OP_IGUALDADE, pno, &type_nulo) == &type_bool);
    assert(tipo_resultado(OP_RELACIONAL, pno, &type_nulo) == &type_erro);
    assert(tipo_resultado(OP_ARITMETICO, pno, pno) == &type_erro);
    assert(tipo_atribuivel(pno, &type_nulo) && !tipo_atribuivel(&type_int, &type_nulo));
    int ep = erros_semanticos();
    Atr chave = verifica_campo(atr("p", pno, 16), "chave", 16);
    assert(chave.tipo == &type_int && strcmp(chave.cod, "((*p)._campo_chave)") == 0);
    assert(erros_semanticos() == ep);

    /* Chamadas e retorno (#13): aridade, tipos (int -> float), ref, proc em expr, return */
    type_init();
    scope_push("global", NULL, 0);
    Subprog *f_real = declare_sub("f_real", 1);
    f_real->is_func = 1;
    f_real->ret = &type_float;
    scope_push("f_real", f_real, 0);
    append_param(f_real, declare_var("a", C_PARAM, &type_float, 0, 1));
    append_param(f_real, declare_var("b", C_PARAM, &type_float, 1, 1));
    scope_pop();

    Subprog *p_vazio = declare_sub("p_vazio", 2);
    scope_pop();

    int e13 = erros_semanticos();
    /* int -> float por valor OK; float -> ref float OK */
    Arg *a1 = arg_novo("1", &type_int, 0, 20);
    a1->next = arg_novo("&y", &type_float, 1, 20);
    Atr ch_ok = verifica_chamada("f_real", a1, 20);
    assert(ch_ok.tipo == &type_float && erros_semanticos() == e13);

    /* int -> ref float é erro (ponteiro incompatível) */
    Arg *a2 = arg_novo("1.5", &type_float, 0, 21);
    a2->next = arg_novo("&x", &type_int, 1, 21);
    verifica_chamada("f_real", a2, 21);
    assert(erros_semanticos() == e13 + 1);

    /* proc em expressão gera erro e propaga type_erro */
    Atr ch_proc = verifica_chamada("p_vazio", NULL, 22);
    assert(ch_proc.tipo == &type_void && erros_semanticos() == e13 + 1);
    Atr proc_expr = verifica_chamada_expr(ch_proc);
    assert(proc_expr.tipo == &type_erro && erros_semanticos() == e13 + 2);

    /* return: int -> float OK em func float; return; em func e return expr em proc são erro */
    verifica_retorno(f_real, 1, atr("1", &type_int, 23), 23);
    assert(erros_semanticos() == e13 + 2);
    verifica_retorno(f_real, 0, atr("", &type_void, 24), 24);
    assert(erros_semanticos() == e13 + 3);
    verifica_retorno(p_vazio, 1, atr("1", &type_int, 25), 25);
    assert(erros_semanticos() == e13 + 4);
    verifica_retorno(p_vazio, 0, atr("", &type_void, 26), 26);
    assert(erros_semanticos() == e13 + 4);

    /* Coerção (#11): int -> float vira cast no C; os demais casos não mudam */
    Atr c1 = coage(atr("x", &type_int, 13), &type_float);
    assert(c1.tipo == &type_float && strcmp(c1.cod, "((double)x)") == 0);
    Atr c2 = coage(atr("y", &type_float, 13), &type_float);
    assert(c2.tipo == &type_float && strcmp(c2.cod, "y") == 0);
    Atr c3 = coage(atr("z", &type_int, 13), &type_int);
    assert(c3.tipo == &type_int && strcmp(c3.cod, "z") == 0);
    Atr mist = atr_bin(atr("1", &type_int, 14), "+", atr("2.5", &type_float, 14), OP_ARITMETICO);
    assert(mist.tipo == &type_float && strcmp(mist.cod, "(((double)1) + 2.5)") == 0);

    puts("tipos: OK");
    return 0;
}
