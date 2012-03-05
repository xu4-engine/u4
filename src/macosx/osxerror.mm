#include <cstdio>
#include <cstdarg>
#include <cstdlib>

/*
 * MacOS X: errors shown in alert panel
 */
#ifdef MACOSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

void errorFatal(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

#ifdef MACOSX
    NSRunCriticalAlertPanel(@"XU4 Error", [NSString stringWithCString:buffer encoding:NSUTF8StringEncoding], @"OK", NULL, NULL);
    exit(1);
#else
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Ultima4 iPad"
                                                    message:[NSString stringWithCString:buffer encoding:NSUTF8StringEncoding]
                              delegate:nil cancelButtonTitle:nil otherButtonTitles:nil] autorelease];
    [alert show];    
#endif
}

void errorWarning(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
#ifdef MACOSX
    NSRunAlertPanel(@"XU4 Error", [NSString stringWithCString:buffer encoding:NSUTF8StringEncoding], @"OK", NULL, NULL);
#else
    UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Ultima4 iPad"
                                                    message:[NSString stringWithCString:buffer encoding:NSUTF8StringEncoding]
                                                   delegate:nil cancelButtonTitle:nil otherButtonTitles:nil] autorelease];
    [alert show];    
#endif
}


