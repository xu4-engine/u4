/*
 * $Id$
 */

#ifndef UTILS_C
#define UTILS_C

#ifdef __cplusplus
extern "C" {
#endif

#define AdjustValueMax(var, val, max) ((var) += (val)); if ((var) > (max)) (var) = (max)
#define AdjustValueMin(var, val, min) ((var) += (val)); if ((var) < (min)) (var) = (min)
#define AdjustValue(var, val, max, min) ((var) += (val)); if ((var) > (max)) (var) = (max); if ((var) < (min)) (var) = (min)

void xu4_srandom(void);
int xu4_random(int upperval);

#ifdef __cplusplus
}
#endif

#endif
