/*
 * $Id$
 */

#ifndef DIRECTION_H
#define DIRECTION_H

typedef enum {
    DIR_WEST,
    DIR_NORTH,
    DIR_EAST,
    DIR_SOUTH
} Direction;

void dirMove(Direction dir, int *x, int *y);
Direction dirReverse(Direction dir);

#endif
