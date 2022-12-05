#ifndef DEBUG_H
#define DEBUG_H

#include <cstdio>

/**
 * Define XU4_FUNCTION as the function name.  Most compilers define
 * __FUNCTION__.  GCC provides __FUNCTION__ as a variable, not as a
 * macro, so detecting with #if __FUNCTION__ doesn't work.
 */
#if defined(__GNUC__) || defined(__FUNCTION__)
#   define XU4_FUNCTION __FUNCTION__
#else
#   define XU4_FUNCTION ""
#endif

/*
 * Derived from XINE_ASSERT in the xine project.  I've updated it to
 * be C99 compliant, to use stderr rather than stdout, and to compile
 * out when NDEBUG is set, like a regular assert.  Finally, an
 * alternate ASSERT stub is provided for pre C99 systems.
 */

void print_trace(FILE *file);

#if HAVE_VARIADIC_MACROS
#   ifdef NDEBUG
#       define ASSERT(exp, desc, ...)  /* nothing */
#   else
#       define ASSERT(exp, ...)                                             \
            do {                                                            \
                if (!(exp)) {                                               \
                    fprintf(stderr, "%s:%s:%d: assertion `%s' failed. ",    \
                           __FILE__, XU4_FUNCTION, __LINE__, #exp);         \
                    fprintf(stderr, __VA_ARGS__);                           \
                    fprintf(stderr, "\n\n");                                \
                    print_trace(stderr);                                    \
                    abort();                                                \
                }                                                           \
            } while(0)
#   endif /* ifdef NDEBUG */
#else
void ASSERT(bool exp, const char *desc, ...);
#endif

#endif /* DEBUG_H */
