#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdint.h>

// --- IO MODULE ---
void abyss_io_print_str(const char* s);
void abyss_io_print_int(int64_t v);
void abyss_io_print_float(double v); // <--- ADDED THIS

// --- MATH MODULE ---
int64_t abyss_math_pow(int64_t base, int64_t exp);
int64_t abyss_math_abs(int64_t val);

#endif
