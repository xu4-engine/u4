/*
 * $Id$
 */

#ifndef UTILS_C
#define UTILS_C

#define AdjustValue(var, val, max) ((var) += (val)); if ((var) > (max)) (var) = (max)

char *concat(const char *str, ...);
int strcmp_i(const char *str1, const char *str2);

#endif