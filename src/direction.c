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
 * another.  This algorithm will avoids getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 */
Direction dirFindPath(int from_x, int from_y, int to_x, int to_y, int valid_directions_mask) {
    int dx, dy;

    dx = from_x - to_x;
    dy = from_y - to_y;

    /*
     * remove directions that move away from the target
     */
    if (dx < 0)
        valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_directions_mask);
    else if (dx > 0)
        valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_directions_mask);
        
    if (dy < 0)
        valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_directions_mask);
    else if (dy > 0)
        valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_directions_mask);

    /*
     * in the case where the source and target share a row or a
     * column, go directly towards the target, if possible; if the
     * direct route is blocked, it falls back on the other directions
     */
    if (dx == 0) {
        if (DIR_IN_MASK(DIR_NORTH, valid_directions_mask) ||
            DIR_IN_MASK(DIR_SOUTH, valid_directions_mask) ||
            dy == 1 || dy == -1) {
            valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_directions_mask);
            valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_directions_mask);
        }
    }
    if (dy == 0) {
        if (DIR_IN_MASK(DIR_EAST, valid_directions_mask) ||
            DIR_IN_MASK(DIR_WEST, valid_directions_mask) ||
            dx == 1 || dx == -1) {
            valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_directions_mask);
            valid_directions_mask = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_directions_mask);
        }
    }

    return dirRandomDir(valid_directions_mask);
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
