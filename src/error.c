/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(__CYGWIN__)

/*
 * Windows: errors shown in message box
 */

#include <windows.h>

void errorFatal(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBox(NULL, buffer, "XU4 Error", MB_OK | MB_ICONERROR);

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBox(NULL, buffer, "XU4 Warning", MB_OK | MB_ICONWARNING);
}

#else

/*
 * non-MS operating systems: errors go to standard error stream
 */

void errorFatal(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: warning: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

#endif
