#include <stdarg.h>

/*
 * MacOS X: errors shown in alert panel
 */

#import <Cocoa/Cocoa.h>

void errorFatal(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    NSRunCriticalAlertPanel(@"XU4 Error", [NSString stringWithCString:buffer], @"OK", NULL, NULL);

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    NSRunAlertPanel(@"XU4 Error", [NSString stringWithCString:buffer], @"OK", NULL, NULL);
}


