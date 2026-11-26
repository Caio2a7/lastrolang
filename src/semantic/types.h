#ifndef TYPES_H
#define TYPES_H

/**
 * @file types.h
 * @brief Descritores de tipo e tabela de tipos (chave = nome do tipo).
 */

/* K_TEXTO: literal de texto, só aceito como argumento de escrever (não há tipo string). */
typedef enum { K_INT, K_FLOAT, K_BOOL, K_VOID, K_ERRO, K_ARRAY, K_RECORD, K_TEXTO } Kind;

/* Declaração antecipada: Field aponta para TypeDesc e TypeDesc aponta para Field.
 * Como os dois só guardam ponteiros, o compilador não precisa do tamanho do
 * outro struct, e o ciclo é válido em C. */
typedef struct TypeDesc TypeDesc;

typedef struct Field {
    char *name;
    TypeDesc *type;
    struct Field *next;   // Próximo campo, na ordem da declaração
} Field;

struct TypeDesc {
    Kind kind;
    char *name;       // "int", "Conta"; NULL em array
    TypeDesc *elem;   // Array: tipo do elemento
    int size;         // Array: número de elementos
    Field *fields;    // Registro: lista de campos
};

/* Primitivos: uma única instância de cada, então podem ser comparados por ponteiro. */
extern TypeDesc type_int, type_float, type_bool, type_void, type_erro;
/* Fora da tabela de tipos: nenhuma declaração pode ter tipo texto. */
extern TypeDesc type_texto;

/**
 * @brief Registra os tipos primitivos da linguagem (int e bool) na tabela.
 */
void type_init(void);

/**
 * @brief Insere um tipo nomeado (registro) na tabela.
 * @return Ponteiro para o tipo inserido, ou NULL se o nome já existir.
 */
TypeDesc *type_insert(const char *name, TypeDesc data);

/**
 * @brief Procura um tipo pelo nome.
 * @return Ponteiro para o tipo, ou NULL se o nome não for um tipo.
 */
TypeDesc *type_lookup(const char *name);

/**
 * @brief Cria o tipo de um array; arrays são anônimos e não entram na tabela.
 */
TypeDesc *type_array(TypeDesc *elem, int size);

/**
 * @brief Acrescenta uma dimensão à direita: int[N] vira int[N][size].
 *        Matriz é array de array, então a nova dimensão envolve o elemento mais interno.
 */
TypeDesc *type_add_dim(TypeDesc *t, int size);

/**
 * @brief Compara dois tipos: primitivos e registros pela identidade do descritor,
 *        arrays pela estrutura (mesmo tamanho e mesmo tipo de elemento).
 * @return 1 se forem iguais, 0 caso contrário. Coerções e o tipo erro ficam
 *         a cargo da análise semântica.
 */
int type_equal(const TypeDesc *a, const TypeDesc *b);

/**
 * @brief Texto do tipo, como "int", "int[10]", "int[2][3]" ou "Conta".
 * @return String nova (deve ser liberada pelo chamador).
 */
char *type_str(const TypeDesc *t);

/**
 * @brief Imprime a tabela de tipos.
 */
void type_print(void);

#endif
