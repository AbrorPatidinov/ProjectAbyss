#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../include/utils.h"
#include "../include/lexer.h"

void fail(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[1;31mError at Line %d, Col %d:\033[0m ", cur.line, cur.col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    if (cur.text) fprintf(stderr, "Context: Near token '%s'\n", cur.text);
    exit(1);
}
