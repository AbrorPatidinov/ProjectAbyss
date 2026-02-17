#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

typedef enum {
    TK_EOF, TK_ID, TK_NUM_INT, TK_NUM_FLOAT, TK_STR,
    TK_INT, TK_FLOAT, TK_CHAR, TK_VOID, TK_STR_TYPE,
    TK_STRUCT, TK_NEW, TK_FREE, TK_EYE,
    TK_IF, TK_ELSE, TK_WHILE, TK_FOR,
    TK_FUNCTION, TK_TRY, TK_CATCH, TK_THROW,
    TK_RETURN, TK_PRINT, TK_PRINT_CHAR,
    TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE, TK_LBRACKET, TK_RBRACKET,
    TK_SEMI, TK_COMMA, TK_ASSIGN, TK_DOT, TK_COLON,
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_MOD,
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_INC, TK_DEC, TK_PLUS_ASSIGN, TK_MINUS_ASSIGN,
    TK_QUESTION, TK_STACK
} TkKind;

typedef struct {
    TkKind kind;
    char *text;
    int64_t ival;
    double fval;
    int line;
    int col;
} Token;

extern Token cur;

void lexer_init(char *source);
void next();
int accept(TkKind k);
void expect(TkKind k);

// --- NEW ERROR REPORTING HELPERS ---
const char* tk_str(TkKind k);
void print_error_context();

#endif
