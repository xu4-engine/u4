/*
 * $Id$
 */

#ifndef SCALE_H
#define SCALE_H

#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _Image;

typedef struct _Image *(*Scaler)(struct _Image *src, int scale, int n);

Scaler scalerGet(FilterType filter);
int scaler3x(FilterType filter);

#ifdef __cplusplus
}
#endif

#endif /* SCALE_H */
