#!/bin/sh
# Testes da issue #24 (make test-conflitos), rodados da raiz do repositório.
# valido/: o binário tem de imprimir, em ordem, os textos dos comentários "// saida:".
# invalido/: o compilador tem de rejeitar, e as primeiras mensagens têm de começar, em
#   ordem, com os textos dos comentários "// erro:" do topo do arquivo.
# dangling_else: na variante sem chaves o else fica com o if mais próximo (shift).
B=build
falhas=0
ok()    { echo "OK    $1"; }
falha() { echo "FALHA $1"; falhas=$((falhas + 1)); }

for f in tests/conflitos/valido/*.txt; do
    sed -n 's|.*// saida: ||p' "$f" > $B/esperado
    if ./$B/compilador < "$f" > $B/conflito.c 2> /dev/null &&
       gcc -Wall -Werror -o $B/conflito $B/conflito.c &&
       ./$B/conflito | diff $B/esperado - > /dev/null
    then ok "$f"; else falha "$f"; fi
done

for f in tests/conflitos/invalido/*.txt; do
    sed -n 's|^// erro: ||p' "$f" > $B/esperado
    if ./$B/compilador < "$f" > /dev/null 2> $B/erro; then
        falha "$f (aceito)"
    elif awk 'NR == FNR { e[n++] = $0; next }
              FNR <= n && index($0, e[FNR - 1]) != 1 { ruim = 1 }
              END { exit ruim || FNR < n }' $B/esperado $B/erro
    then ok "$f"
    else falha "$f: esperado '$(head -n 1 $B/esperado)', veio '$(head -n 1 $B/erro)'"
    fi
done

for caso in 'if (a) if (b) x; else y;|if(a){if(b){x}else{y}}' \
            'if (a) if (b) if (c) x; else y; else z;|if(a){if(b){if(c){x}else{y}}else{z}}'; do
    if [ "$(echo "${caso%%|*}" | ./$B/dangling_else)" = "${caso#*|}" ]
    then ok "dangling_else: ${caso%%|*}"; else falha "dangling_else: ${caso%%|*}"; fi
done

echo "$falhas falha(s)"
[ $falhas -eq 0 ]
