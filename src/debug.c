/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#if HAVE_BACKTRACE
#include <execinfo.h>

/**
 * Get a backtrace and print it to the file.  Note that gcc requires
 * the -rdynamic flag to have access to the actual backtrace symbols;
 * otherwise they will be simple hex offsets.
 */
void print_trace(FILE *file) {
    /* Code Taken from GNU C Library manual */
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    fprintf(file, "Stack trace:\n");

    /* start at one to omit print_trace */
    for (i = 1; i < size; i++) {
        fprintf(file, "%s\n", strings[i]);
    }
    free(strings);
}

#else

/**
 * Stub for systems without access to the stack backtrace.
 */
void print_trace(FILE *file) {
    fprintf(file, "Stack trace not available\n");
}

#endif

#if !HAVE_VARIADIC_MACROS

void ASSERT(int exp, ...) {
}

#endif

