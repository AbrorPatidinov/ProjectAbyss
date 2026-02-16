#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"
#include "../include/utils.h"

static char *src;
static size_t pos = 0;
static int line = 1;
static int col = 1;
Token cur;

void lexer_init(char *source) {
    src = source;
    pos = 0;
    line = 1;
    col = 1;
    cur.text = NULL;
}

void next() {
    if (cur.text) free(cur.text);
    cur.text = NULL;
    while (src[pos] && isspace(src[pos])) {
        if (src[pos] == '\n') { line++; col = 1; } else { col++; }
        pos++;
    }
    cur.line = line; cur.col = col;

    if (!src[pos]) { cur.kind = TK_EOF; return; }
    char *start = src + pos;

    if (src[pos] == '/' && src[pos+1] == '/') {
        while (src[pos] && src[pos] != '\n') pos++;
        next(); return;
    }

    if (isalpha(src[pos])) {
        while (isalnum(src[pos]) || src[pos] == '_') { pos++; col++; }
        int len = (src + pos) - start;
        cur.text = strndup(start, len); // This will now work correctly
        if (!strcmp(cur.text, "int")) cur.kind = TK_INT;
        else if (!strcmp(cur.text, "float")) cur.kind = TK_FLOAT;
        else if (!strcmp(cur.text, "char")) cur.kind = TK_CHAR;
        else if (!strcmp(cur.text, "void")) cur.kind = TK_VOID;
        else if (!strcmp(cur.text, "str")) cur.kind = TK_STR_TYPE;
        else if (!strcmp(cur.text, "struct")) cur.kind = TK_STRUCT;
        else if (!strcmp(cur.text, "new")) cur.kind = TK_NEW;
        else if (!strcmp(cur.text, "stack")) cur.kind = TK_STACK;
        else if (!strcmp(cur.text, "free")) cur.kind = TK_FREE;
        else if (!strcmp(cur.text, "abyss_eye")) cur.kind = TK_EYE;
        else if (!strcmp(cur.text, "if")) cur.kind = TK_IF;
        else if (!strcmp(cur.text, "else")) cur.kind = TK_ELSE;
        else if (!strcmp(cur.text, "while")) cur.kind = TK_WHILE;
        else if (!strcmp(cur.text, "for")) cur.kind = TK_FOR;
        else if (!strcmp(cur.text, "function")) cur.kind = TK_FUNCTION;
        else if (!strcmp(cur.text, "try")) cur.kind = TK_TRY;
        else if (!strcmp(cur.text, "catch")) cur.kind = TK_CATCH;
        else if (!strcmp(cur.text, "throw")) cur.kind = TK_THROW;
        else if (!strcmp(cur.text, "return")) cur.kind = TK_RETURN;
        else if (!strcmp(cur.text, "print")) cur.kind = TK_PRINT;
        else if (!strcmp(cur.text, "print_char")) cur.kind = TK_PRINT_CHAR;
        else if (!strcmp(cur.text, "error")) cur.kind = TK_STR_TYPE;
        else cur.kind = TK_ID;
        return;
    }

    if (isdigit(src[pos])) {
        while (isdigit(src[pos])) { pos++; col++; }
        if (src[pos] == '.') {
            pos++; col++; while (isdigit(src[pos])) { pos++; col++; }
            cur.kind = TK_NUM_FLOAT;
            cur.text = strndup(start, (src+pos)-start);
            cur.fval = strtod(cur.text, NULL);
        } else {
            cur.kind = TK_NUM_INT;
            cur.text = strndup(start, (src+pos)-start);
            cur.ival = strtoll(cur.text, NULL, 10);
        }
        return;
    }

    if (src[pos] == '"') {
        pos++; col++; char *s = src + pos;
        while (src[pos] && src[pos] != '"') {
            if (src[pos] == '\n') { line++; col = 1; } else col++;
            pos++;
        }
        int len = (src + pos) - s;
        if (src[pos] == '"') { pos++; col++; }
        cur.text = strndup(s, len);
        cur.kind = TK_STR;
        return;
    }

    if (!strncmp(src+pos, "==", 2)) { pos+=2; col+=2; cur.kind = TK_EQ; return; }
    if (!strncmp(src+pos, "!=", 2)) { pos+=2; col+=2; cur.kind = TK_NE; return; }
    if (!strncmp(src+pos, "<=", 2)) { pos+=2; col+=2; cur.kind = TK_LE; return; }
    if (!strncmp(src+pos, ">=", 2)) { pos+=2; col+=2; cur.kind = TK_GE; return; }
    if (!strncmp(src+pos, "++", 2)) { pos+=2; col+=2; cur.kind = TK_INC; return; }
    if (!strncmp(src+pos, "--", 2)) { pos+=2; col+=2; cur.kind = TK_DEC; return; }
    if (!strncmp(src+pos, "+=", 2)) { pos+=2; col+=2; cur.kind = TK_PLUS_ASSIGN; return; }
    if (!strncmp(src+pos, "-=", 2)) { pos+=2; col+=2; cur.kind = TK_MINUS_ASSIGN; return; }

    char c = src[pos++]; col++;
    cur.text = malloc(2); cur.text[0] = c; cur.text[1] = 0;
    switch(c) {
        case '(': cur.kind = TK_LPAREN; break;
        case ')': cur.kind = TK_RPAREN; break;
        case '{': cur.kind = TK_LBRACE; break;
        case '}': cur.kind = TK_RBRACE; break;
        case '[': cur.kind = TK_LBRACKET; break;
        case ']': cur.kind = TK_RBRACKET; break;
        case ';': cur.kind = TK_SEMI; break;
        case ',': cur.kind = TK_COMMA; break;
        case '.': cur.kind = TK_DOT; break;
        case ':': cur.kind = TK_COLON; break;
        case '=': cur.kind = TK_ASSIGN; break;
        case '+': cur.kind = TK_PLUS; break;
        case '-': cur.kind = TK_MINUS; break;
        case '*': cur.kind = TK_MUL; break;
        case '/': cur.kind = TK_DIV; break;
        case '%': cur.kind = TK_MOD; break;
        case '<': cur.kind = TK_LT; break;
        case '>': cur.kind = TK_GT; break;
        case '?': cur.kind = TK_QUESTION; break;
        default: fail("Unknown char '%c'", c);
    }
}

int accept(TkKind k) { if (cur.kind == k) { next(); return 1; } return 0; }
void expect(TkKind k) { if (!accept(k)) fail("Expected token kind %d, got %d ('%s')", k, cur.kind, cur.text ? cur.text : "?"); }
