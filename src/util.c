#include "gbadisasm.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal_error(const char* fmt, ...) {
    va_list args;

    fputs("error: ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
    exit(1);
}