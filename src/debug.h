/*
 * $Id$
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <cstdio>

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
                           __FILE__, __FUNCTION__, __LINE__, #exp);         \
                    fprintf(stderr, __VA_ARGS__);                           \
                    fprintf(stderr, "\n\n");                                \
                    print_trace(stderr);                                    \
                    abort();                                                \
                }                                                           \
            } while(0)
#   endif /* ifdef NDEBUG */
#else

void ASSERT(int exp, const char *desc, ...);

#endif /* if HAVE_VARIADIC_MACROS */

#endif /* ifndef DEBUG_H */


