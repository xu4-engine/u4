/*
 * $Id$
 */

#ifndef SCALE_H
#define SCALE_H

#include "settings.h"

struct _Image;

typedef struct _Image *(*Scaler)(struct _Image *src, int scale, int n);

Scaler scalerGet(FilterType filter);
int scaler3x(FilterType filter);

#endif /* SCALE_H */
