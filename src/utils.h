/*
 * $Id$
 */

#ifndef UTILS_C
#define UTILS_C

#define AdjustValueMax(var, val, max) ((var) += (val)); if ((var) > (max)) (var) = (max)
#define AdjustValueMin(var, val, min) ((var) += (val)); if ((var) < (min)) (var) = (min);
#define AdjustValue(var, val, max, min) ((var) += (val)); if ((var) > (max)) (var) = (max); if ((var) < (min)) (var) = (min);

char *concat(const char *str, ...);
int strcmp_i(const char *str1, const char *str2);
void xu4_srandom(void);
int xu4_random(int upperval);

#endif
