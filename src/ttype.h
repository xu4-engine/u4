/*
 * $Id$
 */

#ifndef TTYPE_H
#define TTYPE_H

#define SWAMP_TILE 0x3
#define GRASS_TILE 0x4
#define BRUSH_TILE 0x5
#define FOREST_TILE 0x6
#define HILLS_TILE 0x7
#define BRIDGE_TILE 0x17
#define NORTHBRIDGE_TILE 0x19
#define SOUTHBRIDGE_TILE 0x1a
#define AVATAR_TILE 0x1f
#define CORPSE_TILE 0x38
#define CHEST_TILE 0x3c
#define BRICKFLOOR_TILE 0x3e
#define MOONGATE0_TILE 0x40
#define MOONGATE1_TILE 0x41
#define MOONGATE2_TILE 0x42
#define MOONGATE3_TILE 0x43
#define POISONFIELD_TILE 0x44
#define LIGHTNINGFIELD_TILE 0x45
#define FIREFIELD_TILE 0x46
#define SLEEPFIELD_TILE 0x47
#define BLACK_TILE 0x7e

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
int tileIsFlyable(unsigned char tile);
int tileIsDoor(unsigned char tile);
int tileIsLockedDoor(unsigned char tile);
int tileIsShip(unsigned char tile);
int tileIsHorse(unsigned char tile);
int tileIsBalloon(unsigned char tile);
int tileCanDispel(unsigned char tile);
unsigned int tileGetDirection(unsigned char tile);
void tileSetDirection(unsigned short *tile, unsigned int dir);
int tileCanTalkOver(unsigned char tile);
TileEffect tileGetEffect(unsigned char tile);
TileAnimationStyle tileGetAnimationStyle(unsigned char tile);
void tileAdvanceFrame(unsigned char *tile);
int tileIsOpaque(unsigned char tile);

#endif
