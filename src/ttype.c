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

#define MASK_UNWALKABLE  0x0001
#define MASK_SPEED       0x0006
#define MASK_EFFECT      0x0018
#define MASK_OPAQUE      0x0020
#define MASK_SWIMABLE    0x0040
#define MASK_SAILABLE    0x0080
#define MASK_ANIMATED    0x0100
#define MASK_UNFLYABLE   0x0200
#define MASK_SHIP        0x0400
#define MASK_HORSE       0x0800
#define MASK_BALLOON     0x1000
#define MASK_CANDISPEL   0x2000
#define MASK_MONSTER_UNWALKABLE 0x4000
#define MASK_CANTALKOVER 0x8000
#define MASK_DOOR        0x10000
#define MASK_LOCKEDDOOR  0x20000

/* tile values 0-127 */
int tileInfoLoaded = 0;
unsigned int _ttype_info[128];
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
    } booleanAttributes[] = {
        { "unwalkable", MASK_UNWALKABLE },
        { "opaque", MASK_OPAQUE },
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },
        { "animated", MASK_ANIMATED },
        { "unflyable", MASK_UNFLYABLE },
        { "candispel", MASK_CANDISPEL },
        { "monsterunwalkable", MASK_MONSTER_UNWALKABLE },
        { "cantalkover", MASK_CANTALKOVER },
        { "door", MASK_DOOR },
        { "lockeddoor", MASK_LOCKEDDOOR },
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

        _ttype_info[tile] = 0;

        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) booleanAttributes[i].name), 
                          (const xmlChar *) "true") == 0)
                _ttype_info[tile] |= booleanAttributes[i].mask;
        }

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "slow") == 0)
            _ttype_info[tile] |= SLOW;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "vslow") == 0)
            _ttype_info[tile] |= VSLOW;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "speed"), (const xmlChar *) "vvslow") == 0)
            _ttype_info[tile] |= VVSLOW;
        
        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "fire") == 0)
            _ttype_info[tile] |= EFFECT_FIRE;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "sleep") == 0)
            _ttype_info[tile] |= EFFECT_SLEEP;
        else if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "effect"), (const xmlChar *) "poison") == 0)
            _ttype_info[tile] |= EFFECT_POISON;

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "ship"), (const xmlChar *) "true") == 0) {
            _ttype_info[tile] |= MASK_SHIP;
            if (baseShip == -1)
                baseShip = tile;
        }

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "horse"), (const xmlChar *) "true") == 0) {
            _ttype_info[tile] |= MASK_HORSE;
            if (baseHorse == -1)
                baseHorse = tile;
        }

        if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "balloon"), (const xmlChar *) "true") == 0) {
            _ttype_info[tile] |= MASK_BALLOON;
            if (baseBalloon == -1)
                baseBalloon = tile;
        }

        tile++;
    }

    /* ensure information for all non-monster tiles was loaded */
    assert(tile == 128);

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

int tileTestBit(unsigned char tile, unsigned int mask, int defaultVal) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (_ttype_info[tile] & mask) != 0;
    return defaultVal;
}


int tileIsWalkable(unsigned char tile) {
    if (tile >= STORM_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    return !tileTestBit(tile, MASK_UNWALKABLE, 1);
}

int tileIsMonsterWalkable(unsigned char tile) {
    if (tile >= STORM_TILE &&
        tile <= (STORM_TILE + 1))
        return 1;

    return !(tileTestBit(tile, MASK_UNWALKABLE, 1) || tileTestBit(tile, MASK_MONSTER_UNWALKABLE, 1));
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
    return tileTestBit(tile, MASK_CANDISPEL, 0);
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
    return tileTestBit(tile, MASK_CANTALKOVER, 0);
}

TileSpeed tileGetSpeed(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (TileSpeed) (_ttype_info[tile] & MASK_SPEED);
    return (TileSpeed) 0;
}

TileEffect tileGetEffect(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tile < (sizeof(_ttype_info) / sizeof(_ttype_info[0])))
	return (TileEffect) (_ttype_info[tile] & MASK_EFFECT);
    return (TileEffect) 0;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

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
    return tileTestBit(tile, MASK_OPAQUE, 0);
}

unsigned char tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
