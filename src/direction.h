/*
 * $Id$
 */

#ifndef DIRECTION_H
#define DIRECTION_H

typedef enum {
    DIR_NONE,
    DIR_WEST,
    DIR_NORTH,
    DIR_EAST,
    DIR_SOUTH
} Direction;

#define MASK_DIR(dir) (1 << (dir))
#define MASK_DIR_WEST (1 << DIR_WEST)
#define MASK_DIR_NORTH (1 << DIR_NORTH)
#define MASK_DIR_EAST (1 << DIR_EAST)
#define MASK_DIR_SOUTH (1 << DIR_SOUTH)
#define MASK_DIR_ALL (MASK_DIR_WEST | MASK_DIR_NORTH | MASK_DIR_EAST | MASK_DIR_SOUTH)

#define DIR_IN_MASK(dir,mask) ((1 << (dir)) & (mask))
#define DIR_ADD_TO_MASK(dir,mask) ((1 << (dir)) | (mask))
#define DIR_REMOVE_FROM_MASK(dir,mask) ((~(1 << (dir))) & (mask))

void dirMove(Direction dir, int *x, int *y);
Direction dirReverse(Direction dir);
Direction dirFromMask(int dir_mask);
Direction dirRotateCW(Direction dir);
Direction dirRotateCCW(Direction dir);
Direction dirGetRelativeDirection(int from_x, int from_y, int to_x, int to_y);
Direction dirGetBroadsidesDirs(Direction dir);
Direction dirFindPathToTarget(int from_x, int from_y, int target_x, int target_y, int valid_directions_mask);
Direction dirFindPathAwayFromTarget(int from_x, int from_y, int target_x, int target_y, int valid_directions_mask);
Direction dirRandomDir(int valid_directions_mask);
Direction dirNormalize(Direction orientation, Direction dir);
Direction keyToDirection(int key);
int directionToKey(Direction dir);

#endif
