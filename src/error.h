/*
 * $Id$
 */

#ifndef ERROR_H
#define ERROR_H

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y) /* nothing */
#endif

#if defined(ANDROID) && defined(__cplusplus)
extern "C"
#endif
void errorFatal(const char *fmt, ...) PRINTF_LIKE(1, 2);
void errorWarning(const char *fmt, ...) PRINTF_LIKE(1, 2);

#endif
