#include "../include/utils.h"
#include "../include/lexer.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fail(const char *fmt, ...) {
  // Print Header
  fprintf(stderr, "\n\033[1;31m[FATAL ERROR]\033[0m ");

  // Print Message
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");

  // Print Visual Context
  print_error_context();

  exit(1);
}
