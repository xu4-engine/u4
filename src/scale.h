/*
 * $Id$
 */

#ifndef SCALE_H
#define SCALE_H

class Image;

typedef Image *(*Scaler)(Image *src, int scale, int n);

Scaler scalerGet(int filter);
int scaler3x(int filter);

#endif /* SCALE_H */
