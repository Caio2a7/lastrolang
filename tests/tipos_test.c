/* Teste da issue #9: compatibilidade e tipos de expressões (inclui float e propagação de erro).
 * Rodar com: make test-tipos */
#include "semantic/semantico.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    TypeDesc *res = NULL;
    TypeDesc *arr = type_array(&type_int, 3);

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

    puts("tipos: OK");
    return 0;
}
