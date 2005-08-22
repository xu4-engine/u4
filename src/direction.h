/*
 * $Id$
 */

#ifndef DIRECTION_H
#define DIRECTION_H

enum Direction {
    DIR_NONE,
    DIR_WEST,
    DIR_NORTH,
    DIR_EAST,
    DIR_SOUTH,
    DIR_ADVANCE,
    DIR_RETREAT
};

#define MASK_DIR(dir) (1 << (dir))
#define MASK_DIR_WEST (1 << DIR_WEST)
#define MASK_DIR_NORTH (1 << DIR_NORTH)
#define MASK_DIR_EAST (1 << DIR_EAST)
#define MASK_DIR_SOUTH (1 << DIR_SOUTH)
#define MASK_DIR_ADVANCE (1 << DIR_ADVANCE)
#define MASK_DIR_RETREAT (1 << DIR_RETREAT)
#define MASK_DIR_ALL (MASK_DIR_WEST | MASK_DIR_NORTH | MASK_DIR_EAST | MASK_DIR_EAST | MASK_DIR_SOUTH | MASK_DIR_ADVANCE | MASK_DIR_RETREAT)

#define DIR_IN_MASK(dir,mask) ((1 << (dir)) & (mask))
#define DIR_ADD_TO_MASK(dir,mask) ((1 << (dir)) | (mask))
#define DIR_REMOVE_FROM_MASK(dir,mask) ((~(1 << (dir))) & (mask))

Direction dirReverse(Direction dir);
Direction dirFromMask(int dir_mask);
Direction dirRotateCW(Direction dir);
Direction dirRotateCCW(Direction dir);
int dirGetBroadsidesDirs(Direction dir);
Direction dirRandomDir(int valid_directions_mask);
Direction dirNormalize(Direction orientation, Direction dir);
Direction keyToDirection(int key);
int directionToKey(Direction dir);

#endif
