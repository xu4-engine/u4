/*
 * $Id$
 */

#ifndef TTYPE_H
#define TTYPE_H

typedef enum {
    EFFECT_NONE = 0x00,
    EFFECT_FIRE = 0x04,
    EFFECT_SLEEP = 0x08,
    EFFECT_POISON = 0x0C
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_WESTSHIPFLAG,
    ANIM_EASTSHIPFLAG,
    ANIM_LCBFLAG,
    ANIM_TWOFRAMES,
    ANIM_FOURFRAMES
} TileAnimationStyle;

int tileIsWalkable(unsigned char tile);
int tileIsSlow(unsigned char tile);
int tileIsVslow(unsigned char tile);
int tileIsSailable(unsigned char tile);
int tileIsDoor(unsigned char tile);
int tileIsLockedDoor(unsigned char tile);
int tileIsShip(unsigned char tile);
int tileIsHorse(unsigned char tile);
int tileCanTalkOver(unsigned char tile);
TileEffect tileGetEffect(unsigned char tile);
TileAnimationStyle tileGetAnimationStyle(unsigned char tile);

#endif
