/*
 * $Id$
 */

#include <stddef.h>
#include <assert.h>

#include "direction.h"

/**
 * Adjusts the given coordinates one unit in the given direction.
 */
void dirMove(Direction dir, int *x, int *y) {
    switch (dir) {
    case DIR_WEST:
        (*x)--;
        break;
    case DIR_NORTH:
        (*y)--;
        break;
    case DIR_EAST:
        (*x)++;
        break;
    case DIR_SOUTH:
        (*y)++;
        break;
    default:
        assert(0);              /* shouldn't happen */
    }
}

/**
 * Returns the opposite direction.
 */
Direction dirReverse(Direction dir) {
    switch (dir) {
    case DIR_WEST:
        return DIR_EAST;
    case DIR_NORTH:
        return DIR_SOUTH;
    case DIR_EAST:
        return DIR_WEST;
    case DIR_SOUTH:
        return DIR_NORTH;
    default:
        assert(0);              /* shouldn't happen */
    }

    return (Direction) 0;
}
