/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdlib>

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

#include <cstdarg>

/**
 * Stub for systems without variadic macros.  Unfortunately, this
 * assert won't be very useful.
 */
void ASSERT(int exp, const char *desc, ...) {
#ifndef NDEBUG
    va_list args;
    va_start(args, desc);

    if (!exp) {
        fprintf(stderr, "Assertion failed: ");
        vfprintf(stderr, desc, args);
        fprintf(stderr, "\n");
        abort();
    }

    va_end(args);
#endif
}

#endif

