/*
 * $Id$
 */

#ifndef SCALE_H
#define SCALE_H

#include "settings.h"

class Image;

typedef Image *(*Scaler)(Image *src, int scale, int n);

Scaler scalerGet(FilterType filter);
int scaler3x(FilterType filter);

#endif /* SCALE_H */
