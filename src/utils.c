/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "utils.h"

/**
 * Utility function to concatenate a NULL terminated list of strings
 * together into a newly allocated string.  Derived from glibc
 * documention, "Copying and Concatenation" section.
 */
char *concat(const char *str, ...) {
    va_list ap;
    unsigned int allocated = 1;
    char *result, *p;

    result = (char *) malloc(allocated);
    if (allocated) {
        const char *s;
        char *newp;

        va_start(ap, str);

        p = result;
        for (s = str; s; s = va_arg(ap, const char *)) {
            unsigned int len = strlen(s);

            /* resize the allocated memory if necessary.  */
            if (p + len + 1 > result + allocated) {
                allocated = (allocated + len) * 2;
                newp = (char *) realloc(result, allocated);
                if (!newp) {
                    free(result);
                    return NULL;
                }
                p = newp + (p - result);
                result = newp;
            }

            memcpy(p, s, len);
            p += len;
        }

        /* terminate the result */
        *p++ = '\0';

        /* resize memory to the optimal size.  */
        newp = (char *) realloc(result, p - result);
        if (newp)
            result = newp;

        va_end (ap);
    }

    return result;
}
