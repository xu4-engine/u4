/*
 * $Id$
 */

#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y) /* nothing */
#endif

void errorFatal(const char *fmt, ...) PRINTF_LIKE(1, 2);
void errorWarning(const char *fmt, ...) PRINTF_LIKE(1, 2);

#ifdef __cplusplus
}
#endif

#endif
