/*
 * $Id$
 */

#ifndef TILE_H
#define TILE_H

#include "direction.h"

struct _TileRule;

#define DEEP_WATER_TILE 0x0
#define WATER_TILE 0x1
#define SHALLOW_WATER_TILE 0x2
#define SWAMP_TILE 0x3
#define GRASS_TILE 0x4
#define BRUSH_TILE 0x5
#define FOREST_TILE 0x6
#define HILLS_TILE 0x7
#define DUNGEON_TILE 0x9
#define CITY_TILE 0xa
#define CASTLE_TILE 0xb
#define TOWN_TILE 0xc
#define LCB1_TILE 0xd
#define LCB2_TILE 0xe
#define LCB3_TILE 0xf
#define LADDERUP_TILE 0x1b
#define LADDERDOWN_TILE 0x1c
#define HORSE1_TILE 0x14
#define HORSE2_TILE 0x15
#define BRICKFLOOR_1_TILE 0x16
#define BRIDGE_TILE 0x17
#define BALLOON_TILE 0x18
#define NORTHBRIDGE_TILE 0x19
#define SOUTHBRIDGE_TILE 0x1a
#define SHRINE_TILE 0x1e
#define AVATAR_TILE 0x1f
#define CHEST_TILE 0x3c
#define BOULDER_TILE 0x37
#define CORPSE_TILE 0x38
#define BRICKFLOOR_TILE 0x3e
#define WOODFLOOR_TILE 0x3f
#define MOONGATE0_TILE 0x40
#define MOONGATE1_TILE 0x41
#define MOONGATE2_TILE 0x42
#define MOONGATE3_TILE 0x43
#define POISONFIELD_TILE 0x44
#define LIGHTNINGFIELD_TILE 0x45
#define FIREFIELD_TILE 0x46
#define SLEEPFIELD_TILE 0x47
#define ALTAR_TILE 0x4A
#define LAVA_TILE 0x4C
#define MISSFLASH_TILE 0x4D
#define MAGICFLASH_TILE 0x4E
#define HITFLASH_TILE 0x4F
#define BLACK_TILE 0x7e
#define WALL_TILE 0x7f
#define PIRATE_TILE 0x80
#define WHIRLPOOL_TILE 0x8c

/* attr masks */
#define MASK_SHIP               0x0001
#define MASK_HORSE              0x0002
#define MASK_BALLOON            0x0004
#define MASK_DISPEL             0x0008
#define MASK_TALKOVER           0x0010
#define MASK_DOOR               0x0020
#define MASK_LOCKEDDOOR         0x0040
#define MASK_CHEST              0x0080
#define MASK_ATTACKOVER         0x0100
#define MASK_CANLANDBALLOON     0x0200
#define MASK_REPLACEMENT        0x0400

/* movement masks */
#define MASK_SWIMABLE           0x0001
#define MASK_SAILABLE           0x0002
#define MASK_UNFLYABLE          0x0004
#define MASK_MONSTER_UNWALKABLE 0x0008

typedef enum {
    FAST,
    SLOW,
    VSLOW,
    VVSLOW
} TileSpeed;

typedef enum {
    EFFECT_NONE,
    EFFECT_FIRE,
    EFFECT_SLEEP,
    EFFECT_POISON,
    EFFECT_POISONFIELD,
    EFFECT_ELECTRICITY,
    EFFECT_LAVA
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

typedef struct _Tile {    
    const char *name;
    int id;
    int index;
    int frames;
    unsigned char displayTile; /* FIXME: this will go away soon */
    unsigned char animated; /* FIXME: this will be changed to 'animation' of type TileAnimationStyle */
    unsigned char opaque;
    struct _TileRule *rule;
} Tile;

Tile *tileFindByName(const char *name);
int tileLoadTileInfo(Tile** tiles, int index, void *node);

int tileCanWalkOn(unsigned char tile, Direction d);
int tileCanWalkOff(unsigned char tile, Direction d);
int tileCanAttackOver(unsigned char tile);
int tileCanLandBalloon(unsigned char tile);
int tileIsReplacement(unsigned char tile);
int tileIsWalkable(unsigned char tile);
int tileIsMonsterWalkable(unsigned char tile);
int tileIsDungeonWalkable(unsigned char tile);
int tileIsSwimable(unsigned char tile);
int tileIsSailable(unsigned char tile);
int tileIsWater(unsigned char tile);
int tileIsFlyable(unsigned char tile);
int tileIsDoor(unsigned char tile);
int tileIsLockedDoor(unsigned char tile);
int tileIsChest(unsigned char tile);
unsigned char tileGetChestBase();
int tileIsShip(unsigned char tile);
unsigned char tileGetShipBase();
int tileIsPirateShip(unsigned char tile);
int tileIsHorse(unsigned char tile);
unsigned char tileGetHorseBase();
int tileIsBalloon(unsigned char tile);
unsigned char tileGetBalloonBase();
int tileCanDispel(unsigned char tile);
Direction tileGetDirection(unsigned char tile);
int tileSetDirection(unsigned char *tile, Direction dir);
int tileCanTalkOver(unsigned char tile);
TileSpeed tileGetSpeed(unsigned char tile);
TileEffect tileGetEffect(unsigned char tile);
TileAnimationStyle tileGetAnimationStyle(unsigned char tile);
void tileAdvanceFrame(unsigned char *tile);
int tileIsOpaque(unsigned char tile);
unsigned char tileForClass(int klass);

#endif
