/*
 * $Id$
 */

#include <stdlib.h>

#include "direction.h"

#include "debug.h"
#include "event.h"

Direction dirFindPath(int from_x, int from_y, int to_x, int to_y, int valid_directions_mask, int towards);

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

Direction dirFromMask(int dir_mask) {
    if (dir_mask & MASK_DIR_NORTH) return DIR_NORTH;
    else if (dir_mask & MASK_DIR_EAST) return DIR_EAST;
    else if (dir_mask & MASK_DIR_SOUTH) return DIR_SOUTH;
    else if (dir_mask & MASK_DIR_WEST) return DIR_WEST;
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
 * Returns a mask of directions that indicate where (to_x, to_y) is relative
 * to (from_x, from_y).  For instance, if the object at (to_x, to_y) is
 * northeast of (from_x, from_y), then this function returns
 * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
 */
Direction dirGetRelativeDirection(int from_x, int from_y, int to_x, int to_y) {
    int dx, dy, dirmask;
    
    dirmask = DIR_NONE;
    dx = from_x - to_x;
    dy = from_y - to_y;
    
    /* add x directions that lead towards to_x to the mask */
    if (dx < 0)
        dirmask |= MASK_DIR(DIR_EAST);
    else if (dx > 0)
        dirmask |= MASK_DIR(DIR_WEST);

    /* add y directions that lead towards to_y to the mask */
    if (dy < 0)
        dirmask |= MASK_DIR(DIR_SOUTH);
    else if (dy > 0)
        dirmask |= MASK_DIR(DIR_NORTH);

    /* return the result */
    return dirmask;
}

/**
 * Returns the a mask containing the broadsides directions for a given direction.
 * For instance, dirGetBroadsidesDirs(DIR_NORTH) returns:
 * (MASK_DIR(DIR_WEST) | MASK_DIR(DIR_EAST))
 */
Direction dirGetBroadsidesDirs(Direction dir) {
    int dirmask = MASK_DIR_ALL;
    dirmask = DIR_REMOVE_FROM_MASK(dir, dirmask);
    dirmask = DIR_REMOVE_FROM_MASK(dirReverse(dir), dirmask);

    return dirmask;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 */
Direction dirFindPath(int from_x, int from_y, int to_x, int to_y, int valid_directions_mask, int towards) {
    int directionsToObject;
    
    /* find the directions that lead [to/away from] our target */
    directionsToObject = towards ?
        dirGetRelativeDirection(from_x, from_y, to_x, to_y) :
        ~dirGetRelativeDirection(from_x, from_y, to_x, to_y);
    
    /* make sure we eliminate impossible options */
    directionsToObject &= valid_directions_mask;

    /* get the new direction to move */
    if (directionsToObject > DIR_NONE)
        return dirRandomDir(directionsToObject);
    
    /* there are no valid directions that lead to our target, just move wherever we can! */
    else return dirRandomDir(valid_directions_mask);
}

Direction dirFindPathToTarget(int from_x, int from_y, int target_x, int target_y, int valid_directions_mask) {
    return dirFindPath(from_x, from_y, target_x, target_y, valid_directions_mask, 1);
}

Direction dirFindPathAwayFromTarget(int from_x, int from_y, int target_x, int target_y, int valid_directions_mask) {
    return dirFindPath(from_x, from_y, target_x, target_y, valid_directions_mask, 0);
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
