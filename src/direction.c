/*
 * $Id$
 */

#include <stdlib.h>
#include <assert.h>

#include "direction.h"

/**
 * Adjusts the given coordinates one unit in the given direction.
 */
void dirMove(Direction dir, int *x, int *y) {
    switch (dir) {
    case DIR_NONE:
        break;
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
    case DIR_NONE:
        return DIR_NONE;
    case DIR_WEST:
        return DIR_EAST;
    case DIR_NORTH:
        return DIR_SOUTH;
    case DIR_EAST:
        return DIR_WEST;
    case DIR_SOUTH:
        return DIR_NORTH;
    }

    assert(0);                  /* shouldn't happen */
    return DIR_NONE;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  Quite possibly the worst path finding algorithm ever
 * conceived; causes things to get stuck behind obstacles easily.
 */
Direction dirFindPath(int from_x, int from_y, int to_x, int to_y, int valid_directions_mask) {
    Direction vert_dir, horiz_dir;
    int dx, dy;

    dx = from_x - to_x;
    dy = from_y - to_y;

    vert_dir = (dy < 0) ? DIR_SOUTH : (dy > 0) ? DIR_NORTH : DIR_NONE;
    horiz_dir = (dx < 0) ? DIR_EAST : (dx > 0) ? DIR_WEST : DIR_NONE;

    /* restrict direction to the valid directions specified */
    if (!DIR_IN_MASK(vert_dir, valid_directions_mask))
        vert_dir = DIR_NONE;
    if (!DIR_IN_MASK(horiz_dir, valid_directions_mask))
        horiz_dir = DIR_NONE;

    if (vert_dir == DIR_NONE)
        return horiz_dir;
    else if (horiz_dir == DIR_NONE)
        return vert_dir;
    else
        return (rand() % 2) ? vert_dir : horiz_dir;
}

/**
 * Returns a random direction from a provided mask of available
 * directions.
 */
Direction dirRandomDir(int valid_directions_mask) {
    int i, n;
    Direction d[4];

    n = 0;
    for (i = DIR_WEST; i <= DIR_SOUTH; i++) {
        if (DIR_IN_MASK(i, valid_directions_mask)) {
            d[n] = i;
            n++;
        }
    }

    if (n == 0)
        return DIR_NONE;

    return d[rand() % n];
}
