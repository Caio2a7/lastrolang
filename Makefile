CC      = gcc
LEX     = lex
YACC    = yacc -d -v -t
BUILD   = build
CFLAGS  = -Wall -Isrc -I$(BUILD)
TARGET  = $(BUILD)/compilador
SRCS    = $(wildcard src/semantic/*.c src/gen/*.c)

all: $(TARGET)

$(TARGET): $(BUILD)/y.tab.c $(BUILD)/lex.yy.c $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^ -lfl

$(BUILD)/y.tab.c $(BUILD)/y.tab.h: src/parser/parser.y | $(BUILD)
	$(YACC) -o $(BUILD)/y.tab.c $<

$(BUILD)/lex.yy.c: src/lexer/lexer.l $(BUILD)/y.tab.h | $(BUILD)
	$(LEX) -o $@ $<

$(BUILD):
	mkdir -p $@

# Relatório (LaTeX, modelo SBC) em RELATORIO.pdf na raiz. Compila dentro de build/relatorio
# para os auxiliares não sujarem relatorio/; pdflatex roda 3 vezes (bibtex + sumário + referências).
TEXDIR   = $(BUILD)/relatorio
TEXENV   = TEXINPUTS=$(CURDIR)/relatorio//: BIBINPUTS=$(CURDIR)/relatorio: BSTINPUTS=$(CURDIR)/relatorio:
PDFLATEX = $(TEXENV) pdflatex -interaction=nonstopmode -halt-on-error $(CURDIR)/relatorio/main.tex >/dev/null

doc: RELATORIO.pdf

RELATORIO.pdf: $(wildcard relatorio/*.tex relatorio/*.sty relatorio/*.bst relatorio/*.bib relatorio/secoes/*.tex)
	@mkdir -p $(TEXDIR)
	@cd $(TEXDIR) && { $(PDFLATEX) && $(TEXENV) bibtex -terse main && $(PDFLATEX) && $(PDFLATEX); } \
		|| { echo "erro no LaTeX: veja $(TEXDIR)/main.log"; exit 1; }
	@cp $(TEXDIR)/main.pdf $@
	@echo "gerado: $@"

# Gera o C, compila com gcc -Wall -Werror e executa: make run P=tests/valido/03_funcao_juros.txt
run: all
	./$(TARGET) < $(P) > $(BUILD)/programa.c
	$(CC) -Wall -Werror -o $(BUILD)/programa $(BUILD)/programa.c
	./$(BUILD)/programa

test: all
	@echo "== Casos válidos (C gerado tem de compilar com gcc -Wall -Werror) =="
	@for f in tests/valido/*.txt; do echo "-- $$f --"; ./$(TARGET) < $$f > $(BUILD)/teste.c && $(CC) -Wall -Werror -o $(BUILD)/teste $(BUILD)/teste.c && echo "gcc: OK"; echo; done
	@echo "== Casos inválidos (devem dar erro) =="
	@for f in tests/invalido/*.txt; do echo "-- $$f --"; ./$(TARGET) < $$f; echo; done
	@echo "== Casos semânticos inválidos (devem dar erro) =="
	@for f in tests/semantic/invalido/*.txt; do echo "-- $$f --"; ./$(TARGET) < $$f; echo; done

# Teste em C das tabelas e da pilha de escopos (issues #5 e #6).
test-tabelas: | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/tabelas_test tests/tabelas_test.c $(SRCS)
	./$(BUILD)/tabelas_test

# Teste em C dos tipos de expressões (issue #9).
test-tipos: | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/tipos_test tests/tipos_test.c $(SRCS)
	./$(BUILD)/tipos_test

# Mostra o resumo de conflitos do y.output (gerado pelo -v do yacc),
# útil para conferir contra as tabelas LR(0)/SLR/LR(1)/LALR(1) feitas à mão.
conflitos: $(BUILD)/y.tab.c
	@grep -i "conflict" $(BUILD)/y.output || echo "Nenhum conflito reportado."

clean:
	rm -rf $(BUILD)

.PHONY: all run test test-tabelas test-tipos conflitos clean doc
