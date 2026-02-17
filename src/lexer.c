#include "../include/lexer.h"
#include "../include/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *src;
static size_t pos = 0;
static int line = 1;
static int col = 1;
Token cur;

// --- TOKEN NAMES FOR HUMANS ---
static const char *tk_names[] = {[TK_EOF] = "End of File",
                                 [TK_ID] = "Identifier",
                                 [TK_NUM_INT] = "Integer",
                                 [TK_NUM_FLOAT] = "Float",
                                 [TK_STR] = "String",
                                 [TK_INT] = "int",
                                 [TK_FLOAT] = "float",
                                 [TK_CHAR] = "char",
                                 [TK_VOID] = "void",
                                 [TK_STR_TYPE] = "str",
                                 [TK_STRUCT] = "struct",
                                 [TK_NEW] = "new",
                                 [TK_FREE] = "free",
                                 [TK_EYE] = "abyss_eye",
                                 [TK_IF] = "if",
                                 [TK_ELSE] = "else",
                                 [TK_WHILE] = "while",
                                 [TK_FOR] = "for",
                                 [TK_FUNCTION] = "function",
                                 [TK_TRY] = "try",
                                 [TK_CATCH] = "catch",
                                 [TK_THROW] = "throw",
                                 [TK_RETURN] = "return",
                                 [TK_PRINT] = "print",
                                 [TK_PRINT_CHAR] = "print_char",
                                 [TK_LPAREN] = "(",
                                 [TK_RPAREN] = ")",
                                 [TK_LBRACE] = "{",
                                 [TK_RBRACE] = "}",
                                 [TK_LBRACKET] = "[",
                                 [TK_RBRACKET] = "]",
                                 [TK_SEMI] = ";",
                                 [TK_COMMA] = ",",
                                 [TK_ASSIGN] = "=",
                                 [TK_DOT] = ".",
                                 [TK_COLON] = ":",
                                 [TK_PLUS] = "+",
                                 [TK_MINUS] = "-",
                                 [TK_MUL] = "*",
                                 [TK_DIV] = "/",
                                 [TK_MOD] = "%",
                                 [TK_EQ] = "==",
                                 [TK_NE] = "!=",
                                 [TK_LT] = "<",
                                 [TK_GT] = ">",
                                 [TK_LE] = "<=",
                                 [TK_GE] = ">=",
                                 [TK_INC] = "++",
                                 [TK_DEC] = "--",
                                 [TK_PLUS_ASSIGN] = "+=",
                                 [TK_MINUS_ASSIGN] = "-=",
                                 [TK_QUESTION] = "?",
                                 [TK_STACK] = "stack"};

const char *tk_str(TkKind k) {
  if (k >= 0 && k < sizeof(tk_names) / sizeof(char *)) {
    return tk_names[k] ? tk_names[k] : "Unknown Token";
  }
  return "Unknown Token";
}

// --- VISUAL ERROR CONTEXT ---
void print_error_context() {
  if (!src)
    return;

  // Find start of the current line
  int current_line = 1;
  char *p = src;
  char *line_start = src;

  while (*p && current_line < cur.line) {
    if (*p == '\n') {
      current_line++;
      line_start = p + 1;
    }
    p++;
  }

  // Find end of the current line
  char *line_end = line_start;
  while (*line_end && *line_end != '\n') {
    line_end++;
  }

  // Print the visual context
  fprintf(stderr, "\n\033[1;34m     |\033[0m\n");
  fprintf(stderr, "\033[1;34m %3d |\033[0m %.*s\n", cur.line,
          (int)(line_end - line_start), line_start);
  fprintf(stderr, "\033[1;34m     |\033[0m ");

  // Print spaces to align caret
  for (int i = 1; i < cur.col; i++)
    fprintf(stderr, " ");
  fprintf(stderr, "\033[1;31m^ HERE\033[0m\n\n");
}

void lexer_init(char *source) {
  src = source;
  pos = 0;
  line = 1;
  col = 1;
  cur.text = NULL;
}

void next() {
  if (cur.text)
    free(cur.text);
  cur.text = NULL;
  while (src[pos] && isspace(src[pos])) {
    if (src[pos] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
    pos++;
  }
  cur.line = line;
  cur.col = col;

  if (!src[pos]) {
    cur.kind = TK_EOF;
    return;
  }
  char *start = src + pos;

  if (src[pos] == '/' && src[pos + 1] == '/') {
    while (src[pos] && src[pos] != '\n')
      pos++;
    next();
    return;
  }

  if (isalpha(src[pos])) {
    while (isalnum(src[pos]) || src[pos] == '_') {
      pos++;
      col++;
    }
    int len = (src + pos) - start;
    cur.text = strndup(start, len);
    if (!strcmp(cur.text, "int"))
      cur.kind = TK_INT;
    else if (!strcmp(cur.text, "float"))
      cur.kind = TK_FLOAT;
    else if (!strcmp(cur.text, "char"))
      cur.kind = TK_CHAR;
    else if (!strcmp(cur.text, "void"))
      cur.kind = TK_VOID;
    else if (!strcmp(cur.text, "str"))
      cur.kind = TK_STR_TYPE;
    else if (!strcmp(cur.text, "struct"))
      cur.kind = TK_STRUCT;
    else if (!strcmp(cur.text, "new"))
      cur.kind = TK_NEW;
    else if (!strcmp(cur.text, "stack"))
      cur.kind = TK_STACK;
    else if (!strcmp(cur.text, "free"))
      cur.kind = TK_FREE;
    else if (!strcmp(cur.text, "abyss_eye"))
      cur.kind = TK_EYE;
    else if (!strcmp(cur.text, "if"))
      cur.kind = TK_IF;
    else if (!strcmp(cur.text, "else"))
      cur.kind = TK_ELSE;
    else if (!strcmp(cur.text, "while"))
      cur.kind = TK_WHILE;
    else if (!strcmp(cur.text, "for"))
      cur.kind = TK_FOR;
    else if (!strcmp(cur.text, "function"))
      cur.kind = TK_FUNCTION;
    else if (!strcmp(cur.text, "try"))
      cur.kind = TK_TRY;
    else if (!strcmp(cur.text, "catch"))
      cur.kind = TK_CATCH;
    else if (!strcmp(cur.text, "throw"))
      cur.kind = TK_THROW;
    else if (!strcmp(cur.text, "return"))
      cur.kind = TK_RETURN;
    else if (!strcmp(cur.text, "print"))
      cur.kind = TK_PRINT;
    else if (!strcmp(cur.text, "print_char"))
      cur.kind = TK_PRINT_CHAR;
    else if (!strcmp(cur.text, "error"))
      cur.kind = TK_STR_TYPE;
    else
      cur.kind = TK_ID;
    return;
  }

  if (isdigit(src[pos])) {
    while (isdigit(src[pos])) {
      pos++;
      col++;
    }
    if (src[pos] == '.') {
      pos++;
      col++;
      while (isdigit(src[pos])) {
        pos++;
        col++;
      }
      cur.kind = TK_NUM_FLOAT;
      cur.text = strndup(start, (src + pos) - start);
      cur.fval = strtod(cur.text, NULL);
    } else {
      cur.kind = TK_NUM_INT;
      cur.text = strndup(start, (src + pos) - start);
      cur.ival = strtoll(cur.text, NULL, 10);
    }
    return;
  }

  if (src[pos] == '"') {
    pos++;
    col++;
    char *s = src + pos;
    while (src[pos] && src[pos] != '"') {
      if (src[pos] == '\n') {
        line++;
        col = 1;
      } else
        col++;
      pos++;
    }
    int len = (src + pos) - s;
    if (src[pos] == '"') {
      pos++;
      col++;
    }
    cur.text = strndup(s, len);
    cur.kind = TK_STR;
    return;
  }

  if (!strncmp(src + pos, "==", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_EQ;
    return;
  }
  if (!strncmp(src + pos, "!=", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_NE;
    return;
  }
  if (!strncmp(src + pos, "<=", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_LE;
    return;
  }
  if (!strncmp(src + pos, ">=", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_GE;
    return;
  }
  if (!strncmp(src + pos, "++", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_INC;
    return;
  }
  if (!strncmp(src + pos, "--", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_DEC;
    return;
  }
  if (!strncmp(src + pos, "+=", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_PLUS_ASSIGN;
    return;
  }
  if (!strncmp(src + pos, "-=", 2)) {
    pos += 2;
    col += 2;
    cur.kind = TK_MINUS_ASSIGN;
    return;
  }

  char c = src[pos++];
  col++;
  cur.text = malloc(2);
  cur.text[0] = c;
  cur.text[1] = 0;
  switch (c) {
  case '(':
    cur.kind = TK_LPAREN;
    break;
  case ')':
    cur.kind = TK_RPAREN;
    break;
  case '{':
    cur.kind = TK_LBRACE;
    break;
  case '}':
    cur.kind = TK_RBRACE;
    break;
  case '[':
    cur.kind = TK_LBRACKET;
    break;
  case ']':
    cur.kind = TK_RBRACKET;
    break;
  case ';':
    cur.kind = TK_SEMI;
    break;
  case ',':
    cur.kind = TK_COMMA;
    break;
  case '.':
    cur.kind = TK_DOT;
    break;
  case ':':
    cur.kind = TK_COLON;
    break;
  case '=':
    cur.kind = TK_ASSIGN;
    break;
  case '+':
    cur.kind = TK_PLUS;
    break;
  case '-':
    cur.kind = TK_MINUS;
    break;
  case '*':
    cur.kind = TK_MUL;
    break;
  case '/':
    cur.kind = TK_DIV;
    break;
  case '%':
    cur.kind = TK_MOD;
    break;
  case '<':
    cur.kind = TK_LT;
    break;
  case '>':
    cur.kind = TK_GT;
    break;
  case '?':
    cur.kind = TK_QUESTION;
    break;
  default:
    fail("Unknown char '%c'", c);
  }
}

int accept(TkKind k) {
  if (cur.kind == k) {
    next();
    return 1;
  }
  return 0;
}

// --- IMPROVED EXPECT ---
void expect(TkKind k) {
  if (!accept(k)) {
    fail("Expected '%s' but found '%s'", tk_str(k), tk_str(cur.kind));
  }
}
