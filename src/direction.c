/*
 * $Id$
 */

#include <stdlib.h>

#include "direction.h"

#include "debug.h"
#include "event.h"

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
        ASSERT(0, "invalid direction: %d", dir);
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

    ASSERT(0, "invalid direction: %d", dir);
    return DIR_NONE;
}

Direction dirRotateCW(Direction dir) {
    dir++;
    if (dir > DIR_SOUTH)
        dir = DIR_WEST;
    return dir;
}

Direction dirRotateCCW(Direction dir) {
    dir--;
    if (dir < DIR_WEST)
        dir = DIR_SOUTH;
    return dir;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
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

Direction dirFindPathToEdge(int from_x, int from_y, int width, int height, int valid_directions_mask) {
    int edge_mask = MASK_DIR_ALL;
    int to_x, to_y;

    if (from_x < (width - from_x - 1))
        edge_mask = DIR_REMOVE_FROM_MASK(DIR_EAST, edge_mask);
    else if (from_x > (width - from_x - 1))
        edge_mask = DIR_REMOVE_FROM_MASK(DIR_WEST, edge_mask);
    
    if (from_y < (height - from_y - 1))
        edge_mask = DIR_REMOVE_FROM_MASK(DIR_NORTH, edge_mask);
    else if (from_y > (height - from_y - 1))
        edge_mask = DIR_REMOVE_FROM_MASK(DIR_SOUTH, edge_mask);

    ASSERT(edge_mask != 0, "edge_mask should have at least one valid direction");

    to_x = from_x;
    to_y = from_y;

    switch(dirRandomDir(edge_mask)) {
    case DIR_NONE:
        ASSERT(0, "DIR_NONE returned by dirRandomDir");
        break;
    case DIR_WEST:
        to_x = -1;
        break;
    case DIR_NORTH:
        to_y = -1;
        break;
    case DIR_EAST:
        to_x = width;
        break;
    case DIR_SOUTH:
        to_y = height;
        break;
    }

    return dirFindPath(from_x, from_y, to_x, to_y, valid_directions_mask);
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

/**
 * Translates a keyboard code into a direction
 */
Direction keyToDirection(int key) {        
    switch (key) {
    case U4_UP: return DIR_NORTH;        
    case U4_DOWN: return DIR_SOUTH;
    case U4_LEFT: return DIR_WEST;        
    case U4_RIGHT: return DIR_EAST;    
    default: return DIR_NONE;
    }    
}

/**
 * Translates a direction into a keyboard code
 */
int directionToKey(Direction dir) {
    switch(dir) {    
    case DIR_WEST: return U4_LEFT;
    case DIR_NORTH: return U4_UP;
    case DIR_EAST: return U4_RIGHT;
    case DIR_SOUTH: return U4_DOWN;
    case DIR_NONE:
    default: return 0;
    }    
}
