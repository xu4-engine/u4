/*
 * $Id$
 */

#include <stddef.h>
#include <assert.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "ttype.h"
#include "monster.h"
#include "error.h"
#include "u4file.h"

#define MASK_OPAQUE      0x0001
#define MASK_SWIMABLE    0x0002
#define MASK_SAILABLE    0x0004
#define MASK_ANIMATED    0x0008
#define MASK_UNFLYABLE   0x0010
#define MASK_SHIP        0x0020
#define MASK_HORSE       0x0040
#define MASK_BALLOON     0x0080
#define MASK_DISPEL      0x0100
#define MASK_MONSTER_UNWALKABLE 0x0200
#define MASK_TALKOVER    0x0400
#define MASK_DOOR        0x0800
#define MASK_LOCKEDDOOR  0x1000
#define MASK_CHEST       0x2000

/* tile values 0-127 */
int tileInfoLoaded = 0;
struct {
    unsigned short mask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
} _ttype_info[128];
int baseChest = -1;
int baseShip = -1;
int baseHorse = -1;
int baseBalloon = -1;

void tileLoadInfoFromXml() {
    char *fname;
    xmlDocPtr doc;
    xmlNodePtr root, node;
    int tile, i;
    static const struct {
        const char *name;
        unsigned int mask;
        int *base;
    } booleanAttributes[] = {
        { "opaque", MASK_OPAQUE, NULL },
        { "swimable", MASK_SWIMABLE, NULL },
        { "sailable", MASK_SAILABLE, NULL },
        { "animated", MASK_ANIMATED, NULL },
        { "unflyable", MASK_UNFLYABLE, NULL },
        { "dispel", MASK_DISPEL, NULL },
        { "monsterunwalkable", MASK_MONSTER_UNWALKABLE, NULL },
        { "talkover", MASK_TALKOVER, NULL },
        { "door", MASK_DOOR, NULL },
        { "lockeddoor", MASK_LOCKEDDOOR, NULL },
        { "chest", MASK_CHEST, &baseChest },
        { "ship", MASK_SHIP, &baseShip },
        { "horse", MASK_HORSE, &baseHorse },
        { "balloon", MASK_BALLOON, &baseBalloon }
    };

    tileInfoLoaded = 1;

    fname = u4find_conf("tiles.xml");
    if (!fname)
        errorFatal("unable to open file tiles.xml");
    doc = xmlParseFile(fname);
    if (!doc)
        errorFatal("error parsing tiles.xml");

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tiles") != 0)
        errorFatal("malformed tiles.xml");

    tile = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "tile") != 0)
            continue;

        _ttype_info[tile].mask = 0;
        _ttype_info[tile].speed = FAST;
        _ttype_info[tile].effect = EFFECT_NONE;
        _ttype_info[tile].walkonDirs = MASK_DIR_ALL;
        _ttype_info[tile].walkoffDirs = MASK_DIR_ALL;

        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) booleanAttributes[i].name), 
                          (const xmlChar *) "true") == 0) {
                _ttype_info[tile].mask |= booleanAttributes[i].mask;
                if (booleanAttributes[i].base &&
                    (*booleanAttributes[i].base) == -1)
                    (*booleanAttributes[i].base) = tile;
            }
        }

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkon"), (const xmlChar *) "all") == 0)
            _ttype_info[tile].walkonDirs = 0;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkon"), (const xmlChar *) "west") == 0)
            _ttype_info[tile].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, _ttype_info[tile].walkonDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkon"), (const xmlChar *) "north") == 0)
            _ttype_info[tile].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, _ttype_info[tile].walkonDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkon"), (const xmlChar *) "east") == 0)
            _ttype_info[tile].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, _ttype_info[tile].walkonDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkon"), (const xmlChar *) "south") == 0)
            _ttype_info[tile].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, _ttype_info[tile].walkonDirs);

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkoff"), (const xmlChar *) "all") == 0)
            _ttype_info[tile].walkoffDirs = 0;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkoff"), (const xmlChar *) "west") == 0)
            _ttype_info[tile].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, _ttype_info[tile].walkoffDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkoff"), (const xmlChar *) "north") == 0)
            _ttype_info[tile].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, _ttype_info[tile].walkoffDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkoff"), (const xmlChar *) "east") == 0)
            _ttype_info[tile].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, _ttype_info[tile].walkoffDirs);
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "cantwalkoff"), (const xmlChar *) "south") == 0)
            _ttype_info[tile].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, _ttype_info[tile].walkoffDirs);

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "slow") == 0)
            _ttype_info[tile].speed = SLOW;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "vslow") == 0)
            _ttype_info[tile].speed = VSLOW;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "vvslow") == 0)
            _ttype_info[tile].speed = VVSLOW;
        
        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "fire") == 0)
            _ttype_info[tile].effect = EFFECT_FIRE;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "sleep") == 0)
            _ttype_info[tile].effect = EFFECT_SLEEP;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "poison") == 0)
            _ttype_info[tile].effect = EFFECT_POISON;

        tile++;
    }

    /* ensure information for all non-monster tiles was loaded */
    assert(tile == 128);

    if (baseChest == -1)
        errorFatal("tile attributes: a tile must have the \"chest\" attribute");

    if (baseShip == -1 ||
        !tileIsShip(baseShip + 1) ||
        !tileIsShip(baseShip + 2) ||
        !tileIsShip(baseShip + 3))
        errorFatal("tile attributes: four consecutive tiles must have the \"ship\" attribute");

    if (baseHorse == -1 ||
        !tileIsHorse(baseHorse + 1))
        errorFatal("tile attributes: two consecutive tiles must have the \"horse\" attribute");

    if (baseBalloon == -1)
        errorFatal("tile attributes: a tile must have the \"balloon\" attribute");

    xmlFreeDoc(doc);
}

int tileTestBit(unsigned char tile, unsigned short mask, int defaultVal) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile].mask & mask) != 0;
    return defaultVal;
}


int tileCanWalkOn(unsigned char tile, Direction d) {
    if (tile >= STORM_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
        return DIR_IN_MASK(d, _ttype_info[tile].walkonDirs);
    else
        return 0;
}

int tileCanWalkOff(unsigned char tile, Direction d) {
    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
        return DIR_IN_MASK(d, _ttype_info[tile].walkoffDirs);
    else
        return 1;
}

int tileIsMonsterWalkable(unsigned char tile) {
    if (tile >= STORM_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    return !tileTestBit(tile, MASK_MONSTER_UNWALKABLE, 1);
}

int tileIsSwimable(unsigned char tile) {
    if (tile >= WHIRLPOOL_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    return tileTestBit(tile, MASK_SWIMABLE, 0);
}

int tileIsSailable(unsigned char tile) {
    if (tile >= WHIRLPOOL_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    return tileTestBit(tile, MASK_SAILABLE, 0);
}

int tileIsFlyable(unsigned char tile) {
    return !tileTestBit(tile, MASK_UNFLYABLE, 0);
}

int tileIsDoor(unsigned char tile) {
    return tileTestBit(tile, MASK_DOOR, 0);
}

int tileIsLockedDoor(unsigned char tile) {
    return tileTestBit(tile, MASK_LOCKEDDOOR, 0);
}

int tileIsChest(unsigned char tile) {
    return tileTestBit(tile, MASK_CHEST, 0);
}

unsigned char tileGetChestBase() {
    return baseChest;
}

int tileIsShip(unsigned char tile) {
    return tileTestBit(tile, MASK_SHIP, 0);
}

unsigned char tileGetShipBase() {
    return baseShip;
}

int tileIsPirateShip(unsigned char tile) {
    if (tile >= PIRATE_TILE && tile < (PIRATE_TILE + 4))
        return 1;
    return 0;
}

int tileIsHorse(unsigned char tile) {
    return tileTestBit(tile, MASK_HORSE, 0);
}

unsigned char tileGetHorseBase() {
    return baseHorse;
}

int tileIsBalloon(unsigned char tile) {
    return tileTestBit(tile, MASK_BALLOON, 0);
}

unsigned char tileGetBalloonBase() {
    return baseBalloon;
}

int tileCanDispel(unsigned char tile) {
    return tileTestBit(tile, MASK_DISPEL, 0);
}

Direction tileGetDirection(unsigned char tile) {
    if (tileIsShip(tile))
        return (Direction) (tile - baseShip + DIR_WEST);
    if (tileIsPirateShip(tile))
        return (Direction) (tile - PIRATE_TILE + DIR_WEST);
    else if (tileIsHorse(tile))
        return tile == baseHorse ? DIR_WEST : DIR_EAST;
    else 
        return DIR_WEST;        /* some random default */
}

void tileSetDirection(unsigned short *tile, Direction dir) {
    if (tileIsShip(*tile))
        *tile = baseShip + dir - DIR_WEST;
    else if (tileIsPirateShip(*tile))
        *tile = PIRATE_TILE + dir - DIR_WEST;
    else if (tileIsHorse(*tile))
        *tile = (dir == DIR_WEST ? baseHorse : baseHorse + 1);
}

int tileCanTalkOver(unsigned char tile) {
    return tileTestBit(tile, MASK_TALKOVER, 0);
}

TileSpeed tileGetSpeed(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return _ttype_info[tile].speed;
    return (TileSpeed) 0;
}

TileEffect tileGetEffect(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return _ttype_info[tile].effect;
    return (TileEffect) 0;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])) &&
        (_ttype_info[tile].mask & MASK_ANIMATED) != 0)
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
    return tileTestBit(tile, MASK_OPAQUE, 0);
}

unsigned char tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
