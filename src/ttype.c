/*
 * $Id$
 */

#include <stddef.h>

#include "ttype.h"

#define MASK_WALKABLE 0x03
#define MASK_EFFECT   0x0C
#define MASK_FLYABLE  0x10
#define MASK_OPAQUE   0x20
#define MASK_SAILABLE 0x40
#define MASK_ANIMATED 0x80

#define WALKABLE      0x00
#define SLOW          0x01
#define VSLOW         0x02
#define UNWALKABLE    0x03

/* tile values 0-31 */
static const char _ttype_info1[] = {
/* sea,  water, shallows, swamp, grass, brush, forest, hills, mountain, */
   0xC3, 0xC3,  0x83,     0x0C,  0x00,  0x01,  0x02,   0x02,  0x03,
/* dungeon, city, castle, town, lcb1, lcb2, lcb3, wship, nship, eship, sship */
   0x00,    0x00, 0x00,   0x00, 0x03, 0x00, 0x03, 0x00,  0x00,  0x00,  0x00,
/* whorse, ehorse, hex,  bridge, baloon, nbridge, sbridge, uladder, dladder */
   0x00,   0x00,   0x00, 0x00,   0x00,   0x00,    0x00,    0x00,    0x00,
/* ruins, shrine, avatar */
   0x00,  0x00,   0x03
};

/* tile values 56-79 */
static const char _ttype_info2[] = {
/* corpse, stonewall, ldoor, door, chest, anhk, brick, wood */
   0x03,   0x03,      0x03,  0x03, 0x00,  0x03, 0x00,  0x00,
/* mgate0, mgate1, mgate2, mgate3, posion field, energy field, fire field, sleep field */
   0x00,   0x00,   0x00,   0x00,   0x8C,         0x83,         0x84,       0x88,
/* solid, sdoor, altar, roast, lava, stone, orb1, orb2 */
   0x03,  0x00,  0x00,  0x03,  0x80, 0x00,  0x00, 0x00
};


int tileIsWalkable(unsigned char tile) {
    if (tile < 32)
	return (_ttype_info1[tile] & MASK_WALKABLE) != 0x03;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) != 0x03;
    return 0;
}

int tileIsSlow(unsigned char tile) {
    if (tile < sizeof(_ttype_info1))
	return (_ttype_info1[tile] & MASK_WALKABLE) == 0x01;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) == 0x01;
    return 0;
}

int tileIsVslow(unsigned char tile) {
    if (tile < sizeof(_ttype_info1))
	return (_ttype_info1[tile] & MASK_WALKABLE) == 0x02;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) == 0x02;
    return 0;
}

int tileIsSailable(unsigned char tile) {
    if (tile < 32)
	return (_ttype_info1[tile] & MASK_SAILABLE) != 0;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_SAILABLE) != 0;
    return 0;
}

int tileIsDoor(unsigned char tile) {
    return tile == 59;
}

int tileIsLockedDoor(unsigned char tile) {
    return tile == 58;
}

int tileIsShip(unsigned char tile) {
    return tile >= 16 && tile < 20;
}

int tileIsHorse(unsigned char tile) {
    return tile >= 20 && tile < 22;
}

int tileCanTalkOver(unsigned char tile) {
    return tile >= 96 && tile <= 122;
}

TileEffect tileGetEffect(unsigned char tile) {
    if (tile < 32)
	return (_ttype_info1[tile] & MASK_EFFECT);
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_EFFECT);
    return 0;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    if (tile < 32 && (_ttype_info1[tile] & MASK_ANIMATED) != 0)
        return ANIM_SCROLL;
    else if (tile >= 56 && tile < 79 && (_ttype_info2[tile - 56] & MASK_ANIMATED) != 0)
	return ANIM_SCROLL;
    else if (tile == 75)
        return ANIM_CAMPFIRE;
    else if (tile == 10)
        return ANIM_CITYFLAG;
    else if (tile == 11)
        return ANIM_CASTLEFLAG;
    else if (tile == 16)
        return ANIM_WESTSHIPFLAG;
    else if (tile == 18)
        return ANIM_EASTSHIPFLAG;
    else if (tile == 14)
        return ANIM_LCBFLAG;
    else if ((tile >= 32 && tile < 48) ||
             (tile >= 80 && tile < 96) ||
             (tile >= 132 && tile < 144))
        return ANIM_TWOFRAMES;
    else if (tile >= 144)
        return ANIM_FOURFRAMES;

    return ANIM_NONE;
}

