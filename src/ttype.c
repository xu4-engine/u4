/*
 * $Id$
 */

#include <stddef.h>

#include "ttype.h"
#include "monster.h"

#define MASK_UNWALKABLE 0x0001
#define MASK_SPEED     0x0006
#define MASK_EFFECT    0x0018
#define MASK_OPAQUE    0x0020
#define MASK_SWIMABLE  0x0040
#define MASK_SAILABLE  0x0080
#define MASK_ANIMATED  0x0100
#define MASK_UNFLYABLE 0x0200
#define MASK_SHIP      0x0400
#define MASK_HORSE     0x0800
#define MASK_BALLOON   0x1000
#define MASK_CANDISPEL 0x2000

#define UNWALKABLE 0x0001

/* tile values 0-79 */
static const short _ttype_info[] = {
    UNWALKABLE | MASK_SWIMABLE | MASK_SAILABLE | MASK_ANIMATED, /* sea */
    UNWALKABLE | MASK_SWIMABLE | MASK_SAILABLE | MASK_ANIMATED, /* water */
    UNWALKABLE | MASK_SWIMABLE | MASK_ANIMATED,                 /* shallows */
    SLOW | EFFECT_POISON,                                       /* swamp */
    0,                                                          /* grass */
    VSLOW,                                                      /* brush */
    VSLOW | MASK_OPAQUE,                                        /* forest */
    VVSLOW,                                                     /* hills */
    UNWALKABLE | MASK_OPAQUE | MASK_UNFLYABLE,                  /* mountains */
    0, 0, 0, 0,                                                 /* dungeon, city, castle, town */
    UNWALKABLE,                                                 /* lcb1 */
    0,                                                          /* lcb2 */
    UNWALKABLE,                                                 /* lcb3 */
    MASK_SHIP,                                                  /* west ship */
    MASK_SHIP,                                                  /* north ship */
    MASK_SHIP,                                                  /* east ship */
    MASK_SHIP,                                                  /* south ship */
    MASK_HORSE,                                                 /* west horse */
    MASK_HORSE,                                                 /* east horse */
    0,                                                          /* hex */
    0,                                                          /* bridge */
    MASK_BALLOON,                                               /* balloon */
    0, 0,                                                       /* nbridge, sbridge */
    0, 0,                                                       /* uladder, dladder */
    0,                                                          /* ruins */
    0,                                                          /* shrine */
    UNWALKABLE,                                                 /* avatar */
    UNWALKABLE,                                                 /* mage1 */
    UNWALKABLE,                                                 /* mage2 */
    UNWALKABLE,                                                 /* bard1 */
    UNWALKABLE,                                                 /* bard2 */
    UNWALKABLE,                                                 /* fighter1 */
    UNWALKABLE,                                                 /* fighter2 */
    UNWALKABLE,                                                 /* druid1 */
    UNWALKABLE,                                                 /* druid2 */
    UNWALKABLE,                                                 /* tinker1 */
    UNWALKABLE,                                                 /* tinker2 */
    UNWALKABLE,                                                 /* paladin1 */
    UNWALKABLE,                                                 /* paladin2 */
    UNWALKABLE,                                                 /* ranger1 */
    UNWALKABLE,                                                 /* ranger2 */
    UNWALKABLE,                                                 /* shepherd1 */
    UNWALKABLE,                                                 /* shepherd2 */
    UNWALKABLE,                                                 /* column */
    UNWALKABLE,                                                 /* solid SW */
    UNWALKABLE,                                                 /* solid SE */
    UNWALKABLE,                                                 /* solid NW */
    UNWALKABLE,                                                 /* solid NE */
    UNWALKABLE,                                                 /* shipmast */
    UNWALKABLE,                                                 /* shipwheel */
    UNWALKABLE,                                                 /* rocks */
    UNWALKABLE,                                                 /* corpse */
    UNWALKABLE | MASK_OPAQUE,                                   /* stonewall */
    UNWALKABLE,                                                 /* ldoor */
    UNWALKABLE,                                                 /* door */
    0,                                                          /* chest */
    UNWALKABLE,                                                 /* ankh */
    0,                                                          /* brickfloor */
    0,                                                          /* woodfloor */
    0,                                                          /* mgate0 */
    0,                                                          /* mgate1 */
    0,                                                          /* mgate2 */
    0,                                                          /* mgate3 */
    EFFECT_POISON | MASK_ANIMATED | MASK_CANDISPEL,             /* poison field */
    UNWALKABLE | MASK_ANIMATED | MASK_CANDISPEL,                /* energy field */
    VVSLOW | EFFECT_FIRE | MASK_ANIMATED | MASK_CANDISPEL,      /* fire field */
    EFFECT_SLEEP | MASK_ANIMATED | MASK_CANDISPEL,              /* sleep field */
    UNWALKABLE,                                                 /* solid */
    MASK_OPAQUE,                                                /* secret door */
    0,                                                          /* altar */
    UNWALKABLE,                                                 /* roast */
    UNWALKABLE | MASK_ANIMATED,                                 /* lava */
    0,                                                          /* miss flash */
    0,                                                          /* magic flash */
    0                                                           /* hit flash */
};

int tileIsWalkable(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_UNWALKABLE) == 0;
    return 0;
}

int tileIsSwimable(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_SWIMABLE) != 0;
    return 0;
}

int tileIsSailable(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_SAILABLE) != 0;
    return 0;
}

int tileIsFlyable(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_UNFLYABLE) == 0;
    return 1;
}

int tileIsDoor(unsigned char tile) {
    return tile == 59;
}

int tileIsLockedDoor(unsigned char tile) {
    return tile == 58;
}

int tileIsShip(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_SHIP) != 0;
    return 0;
}

int tileIsPirateShip(unsigned char tile) {
    if (tile >= PIRATE_TILE && tile < (PIRATE_TILE + 4))
        return 1;
    return 0;
}

int tileIsHorse(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_HORSE) != 0;
    return 0;
}

int tileIsBalloon(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_BALLOON) != 0;
    return 0;
}

int tileCanDispel(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_CANDISPEL) != 0;
    return 0;
}

Direction tileGetDirection(unsigned char tile) {
    if (tileIsShip(tile))
        return (Direction) (tile - 16 + DIR_WEST);
    if (tileIsPirateShip(tile))
        return (Direction) (tile - PIRATE_TILE + DIR_WEST);
    else if (tileIsHorse(tile))
        return tile == 20 ? DIR_WEST : DIR_EAST;
    else 
        return DIR_WEST;        /* some random default */
}

void tileSetDirection(unsigned short *tile, Direction dir) {
    if (tileIsShip(*tile))
        *tile = 16 + dir - DIR_WEST;
    else if (tileIsPirateShip(*tile))
        *tile = PIRATE_TILE + dir - DIR_WEST;
    else if (tileIsHorse(*tile))
        *tile = (dir == DIR_WEST ? 20 : 21);
}

int tileCanTalkOver(unsigned char tile) {
    return tile >= 96 && tile <= 122;
}

TileSpeed tileGetSpeed(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (TileSpeed) (_ttype_info[tile] & MASK_SPEED);
    return (TileSpeed) 0;
}

TileEffect tileGetEffect(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (TileEffect) (_ttype_info[tile] & MASK_EFFECT);
    return (TileEffect) 0;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])) &&
        (_ttype_info[tile] & MASK_ANIMATED) != 0)
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

void tileAdvanceFrame(unsigned char *tile) {
    TileAnimationStyle style = tileGetAnimationStyle(*tile);

    if (style == ANIM_TWOFRAMES) {
        if ((*tile) % 2)
            (*tile)--;
        else
            (*tile)++;
    }
    else if (style == ANIM_FOURFRAMES) {
        if ((*tile) % 4 == 3)
            (*tile) &= ~(0x3);
        else
            (*tile)++;
    }
}

int tileIsOpaque(unsigned char tile) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & MASK_OPAQUE) != 0;
    return tile == 127;         /* brick wall */
}

unsigned char tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
