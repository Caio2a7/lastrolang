CC      = gcc
LEX     = lex
YACC    = yacc -d -v
CFLAGS  = -Wall -Isrc -Iinclude
TARGET  = compilador

all: $(TARGET)

$(TARGET): y.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o $(TARGET) y.tab.c lex.yy.c -lfl

y.tab.c y.tab.h: src/parser.y
	$(YACC) src/parser.y

lex.yy.c: src/lexer.l y.tab.h
	$(LEX) src/lexer.l

test: all
	@echo "== Casos válidos =="
	@for f in tests/valido/*.txt; do echo "-- $$f --"; ./$(TARGET) < $$f; echo; done
	@echo "== Casos inválidos (devem dar erro) =="
	@for f in tests/invalido/*.txt; do echo "-- $$f --"; ./$(TARGET) < $$f; echo; done

# Mostra o resumo de conflitos do y.output (gerado pelo -v do yacc),
# útil para conferir contra as tabelas LR(0)/SLR/LR(1)/LALR(1) feitas à mão.
conflitos: y.tab.c
	@grep -i "conflict" y.output || echo "Nenhum conflito reportado."

clean:
	rm -f y.tab.c y.tab.h y.output lex.yy.c $(TARGET)

.PHONY: all test conflitos clean
