/*
 * $Id$
 */

#include <stddef.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "ttype.h"

#include "context.h"
#include "error.h"
#include "monster.h"
#include "xml.h"

/* attr masks */
#define MASK_OPAQUE             0x0001
#define MASK_ANIMATED           0x0002
#define MASK_SHIP               0x0004
#define MASK_HORSE              0x0008
#define MASK_BALLOON            0x0010
#define MASK_DISPEL             0x0020
#define MASK_TALKOVER           0x0040
#define MASK_DOOR               0x0080
#define MASK_LOCKEDDOOR         0x0100
#define MASK_CHEST              0x0200
#define MASK_ATTACKOVER         0x0400
#define MASK_CANLANDBALLOON     0x0800
#define MASK_REPLACEMENT        0x1000

/* movement masks */
#define MASK_SWIMABLE           0x0001
#define MASK_SAILABLE           0x0002
#define MASK_UNFLYABLE          0x0004
#define MASK_MONSTER_UNWALKABLE 0x0008

/* tile values 0-127 */
int tileInfoLoaded = 0;
Tile _ttype_info[256];
Tile _dng_ttype_info[256];
int baseChest = -1;
int baseShip = -1;
int baseHorse = -1;
int baseBalloon = -1;
int mapTile = 0,
    dngTile = 0,
    subTile = 0;

int tileLoadTileInfo(xmlNodePtr node);
int tileLoadProperties(Tile *tileset, int index, xmlNodePtr node);

Tile *tileCurrentTilesetInfo() {
    return (c && c->location) ? c->location->tileset_info : _ttype_info;
}

/**
 * Load tile information from xml.
 */
void tileLoadInfoFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node, child;    

    tileInfoLoaded = 1;

    doc = xmlParse("tiles.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tiles") != 0)
        errorFatal("malformed tiles.xml");
    
    for (node = root->xmlChildrenNode; node; node = node->next) {
        /* load tile info from the xml node */
        tileLoadTileInfo(node);
        
        /* load children info if possible */
        for (child = node->xmlChildrenNode; child; child = child->next)
            tileLoadTileInfo(child);
    }

    /* ensure information for all non-monster tiles was loaded */
    if (mapTile != 128)
        errorFatal("tiles.xml contained %d entries (must be 128)\n", mapTile);

    /* initialize the values for the monster tiles */
    for ( ; mapTile < sizeof(_ttype_info) / sizeof(_ttype_info[0]); mapTile++) {
        _ttype_info[mapTile].mask = 0;
        _ttype_info[mapTile].movementMask = 0;
        _ttype_info[mapTile].speed = FAST;
        _ttype_info[mapTile].effect = EFFECT_NONE;
        _ttype_info[mapTile].walkonDirs = 0;
        _ttype_info[mapTile].walkoffDirs = MASK_DIR_ALL;
    }    

    if (baseChest == -1)
        errorFatal("tile attributes: a tile must have the \"chest\" attribute");

    if (baseShip == -1 ||
        !tileIsShip((unsigned char)(baseShip + 1)) ||
        !tileIsShip((unsigned char)(baseShip + 2)) ||
        !tileIsShip((unsigned char)(baseShip + 3)))
        errorFatal("tile attributes: four consecutive tiles must have the \"ship\" attribute");

    if (baseHorse == -1 ||
        !tileIsHorse((unsigned char)(baseHorse + 1)))
        errorFatal("tile attributes: two consecutive tiles must have the \"horse\" attribute");

    if (baseBalloon == -1)
        errorFatal("tile attributes: a tile must have the \"balloon\" attribute");

    xmlFreeDoc(doc);
}

/**
 * Loads tile information from the xml node 'node', if it
 * is a valid tile node.  This loads in both <tile> and 
 * <dngTile> nodes.
 */
int tileLoadTileInfo(xmlNodePtr node) {
    Tile *current;
    int *index;
    int lshift;
    int offset;
    int realIndex;        

    /* ignore 'text' nodes */        
    if (xmlNodeIsText(node))
        return 1;

    /* a standard map tile */
    else if (xmlStrcmp(node->name, (const xmlChar *) "tile") == 0) {
        current = _ttype_info;
        index = &mapTile;
        lshift = 0; /* count by 1 */
        offset = 0;
    }
    /* a dungeon tile */
    else if (xmlStrcmp(node->name, (const xmlChar *) "dngTile") == 0) {
        current = _dng_ttype_info;
        index = &dngTile;
        lshift = 4; /* count by 16, turns 0x1 into 0x10, 0x2 into 0x20, etc */
        offset = 0;

        /* subtile of dungeon tile (for example, magic fields - poison, energy, fire, sleep) */
        if (xmlStrcmp(node->parent->name, (const xmlChar *) "dngTile") == 0) {            
            offset = ((*index) - 1) << lshift; /* offsets to 0x90, 0x91, 0x92, etc */
            index = &subTile;            
            lshift = 0;
        }
        else subTile = 0; /* reset subtile if this is a normal dngTile */
    }
    else return 0;

    /* figure out what our real index is going to be for this tile */
    realIndex = ((*index) << lshift) + offset;

    /* load the properties for the tile! */
    tileLoadProperties(current, realIndex, node);    

    /* fill in blank values with duplicates of what we just created */
    if (lshift > 0) {
        int j;
        for (j = 0; j < (1<<lshift)-1; j++)
            memcpy(&current[realIndex]+j+1, &current[realIndex], sizeof(Tile));
    }            

    (*index)++;
    return 1;
}

/**
 * Load properties for the current xml <tile> or <dngTile>
 * node into the appropriate tileset info structure.
 */
int tileLoadProperties(Tile *tileset, int index, xmlNodePtr node) {
    int i;
    
    static const struct {
        const char *name;
        unsigned int mask;
        int *base;
    } booleanAttributes[] = {
        { "opaque", MASK_OPAQUE, NULL },
        { "animated", MASK_ANIMATED, NULL },
        { "dispel", MASK_DISPEL, NULL },
        { "talkover", MASK_TALKOVER, NULL },
        { "door", MASK_DOOR, NULL },
        { "lockeddoor", MASK_LOCKEDDOOR, NULL },
        { "chest", MASK_CHEST, &baseChest },
        { "ship", MASK_SHIP, &baseShip },
        { "horse", MASK_HORSE, &baseHorse },
        { "balloon", MASK_BALLOON, &baseBalloon },
        { "canattackover", MASK_ATTACKOVER, NULL },
        { "canlandballoon", MASK_CANLANDBALLOON, NULL },
        { "replacement", MASK_REPLACEMENT, NULL }
    };

    static const struct {
        const char *name;
        unsigned int mask;      
    } movementBooleanAttr[] = {        
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },       
        { "unflyable", MASK_UNFLYABLE },       
        { "monsterunwalkable", MASK_MONSTER_UNWALKABLE }        
    };

    tileset[index].mask = 0;
    tileset[index].movementMask = 0;
    tileset[index].speed = FAST;
    tileset[index].effect = EFFECT_NONE;
    tileset[index].walkonDirs = MASK_DIR_ALL;
    tileset[index].walkoffDirs = MASK_DIR_ALL;
    tileset[index].displayTile = index; /* itself */

    if (xmlPropExists(node, "displayTile"))    
        tileset[index].displayTile = (unsigned char)xmlGetPropAsInt(node, "displayTile");

    for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
        if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
            tileset[index].mask |= booleanAttributes[i].mask;
            if (booleanAttributes[i].base &&
                (*booleanAttributes[i].base) == -1)
                (*booleanAttributes[i].base) = index;
        }
    }

    for (i = 0; i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]); i++) {
        if (xmlGetPropAsBool(node, movementBooleanAttr[i].name))
            tileset[index].movementMask |= movementBooleanAttr[i].mask;
    }

    if (xmlPropCmp(node, "cantwalkon", "all") == 0)
        tileset[index].walkonDirs = 0;
    else if (xmlPropCmp(node, "cantwalkon", "west") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, tileset[index].walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "north") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, tileset[index].walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "east") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, tileset[index].walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "south") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, tileset[index].walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "advance") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, tileset[index].walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "retreat") == 0)
        tileset[index].walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, tileset[index].walkonDirs);

    if (xmlPropCmp(node, "cantwalkoff", "all") == 0)
        tileset[index].walkoffDirs = 0;
    else if (xmlPropCmp(node, "cantwalkoff", "west") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, tileset[index].walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "north") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, tileset[index].walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "east") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, tileset[index].walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "south") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, tileset[index].walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "advance") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, tileset[index].walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "retreat") == 0)
        tileset[index].walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, tileset[index].walkoffDirs);

    if (xmlPropCmp(node, "speed", "slow") == 0)
        tileset[index].speed = SLOW;
    else if (xmlPropCmp(node, "speed", "vslow") == 0)
        tileset[index].speed = VSLOW;
    else if (xmlPropCmp(node, "speed", "vvslow") == 0)
        tileset[index].speed = VVSLOW;

    if (xmlPropCmp(node, "effect", "fire") == 0)
        tileset[index].effect = EFFECT_FIRE;
    else if (xmlPropCmp(node, "effect", "sleep") == 0)
        tileset[index].effect = EFFECT_SLEEP;
    else if (xmlPropCmp(node, "effect", "poison") == 0)
        tileset[index].effect = EFFECT_POISON;
    else if (xmlPropCmp(node, "effect", "poisonField") == 0)
        tileset[index].effect = EFFECT_POISONFIELD;
    else if (xmlPropCmp(node, "effect", "electricity") == 0)
        tileset[index].effect = EFFECT_ELECTRICITY;
    else if (xmlPropCmp(node, "effect", "lava") == 0)
        tileset[index].effect = EFFECT_LAVA;    

    return 1;
}

int tileTestBit(unsigned char tile, unsigned short mask) {
    Tile *tileset = tileCurrentTilesetInfo();
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    return (tileset[tile].mask & mask) != 0;
}

int tileTestMovementBit(unsigned char tile, unsigned short mask) {
    Tile *tileset = tileCurrentTilesetInfo();
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    return (tileset[tile].movementMask & mask) != 0;
}

int tileCanWalkOn(unsigned char tile, Direction d) {
    Tile *tileset = tileCurrentTilesetInfo();
    return DIR_IN_MASK(d, tileset[tile].walkonDirs);
}

int tileCanWalkOff(unsigned char tile, Direction d) {
    Tile *tileset = tileCurrentTilesetInfo();
    return DIR_IN_MASK(d, tileset[tile].walkoffDirs);
}

int tileCanAttackOver(unsigned char tile) {    
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */
    return tileIsWalkable(tile) || tileIsSwimable(tile) || tileIsSailable(tile) ||       
        tileTestBit(tile, MASK_ATTACKOVER);
}

int tileCanLandBalloon(unsigned char tile) {
    return tileTestBit(tile, MASK_CANLANDBALLOON);
}

int tileIsReplacement(unsigned char tile) {
    return tileTestBit(tile, MASK_REPLACEMENT);
}

int tileIsWalkable(unsigned char tile) {
    Tile *tileset = tileCurrentTilesetInfo();
    return tileset[tile].walkonDirs > 0;
}

int tileIsMonsterWalkable(unsigned char tile) {
    return !tileTestMovementBit(tile, MASK_MONSTER_UNWALKABLE);
}

int tileIsSwimable(unsigned char tile) {
    return tileTestMovementBit(tile, MASK_SWIMABLE);
}

int tileIsSailable(unsigned char tile) {
    return tileTestMovementBit(tile, MASK_SAILABLE);
}

int tileIsWater(unsigned char tile) {
    return (tileIsSwimable(tile) | tileIsSailable(tile));
}

int tileIsFlyable(unsigned char tile) {
    return !tileTestMovementBit(tile, MASK_UNFLYABLE);
}

int tileIsDoor(unsigned char tile) {
    return tileTestBit(tile, MASK_DOOR);
}

int tileIsLockedDoor(unsigned char tile) {
    return tileTestBit(tile, MASK_LOCKEDDOOR);
}

int tileIsChest(unsigned char tile) {
    return tileTestBit(tile, MASK_CHEST);
}

unsigned char tileGetChestBase() {
    return baseChest;
}

int tileIsShip(unsigned char tile) {
    return tileTestBit(tile, MASK_SHIP);
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
    return tileTestBit(tile, MASK_HORSE);
}

unsigned char tileGetHorseBase() {
    return baseHorse;
}

int tileIsBalloon(unsigned char tile) {
    return tileTestBit(tile, MASK_BALLOON);
}

unsigned char tileGetBalloonBase() {
    return baseBalloon;
}

int tileCanDispel(unsigned char tile) {
    return tileTestBit(tile, MASK_DISPEL);
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

int tileSetDirection(unsigned char *tile, Direction dir) {
    int newDir = 1;
    int oldTile = *tile;

    /* Make sure we even have a direction */
    if (dir <= DIR_NONE)
        return 0;

    if (tileIsShip(*tile))
        *tile = baseShip + dir - DIR_WEST;
    else if (tileIsPirateShip(*tile))
        *tile = PIRATE_TILE + dir - DIR_WEST;
    else if (tileIsHorse(*tile))
        *tile = (dir == DIR_WEST ? baseHorse : baseHorse + 1);
    else   
        newDir = 0;

    if (oldTile == *tile)
        newDir = 0;

    return newDir;
}

int tileCanTalkOver(unsigned char tile) {
    return tileTestBit(tile, MASK_TALKOVER);
}

TileSpeed tileGetSpeed(unsigned char tile) {
    Tile *tileset = tileCurrentTilesetInfo();
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    return tileset[tile].speed;
}

TileEffect tileGetEffect(unsigned char tile) {
    Tile *tileset = tileCurrentTilesetInfo();
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    return tileset[tile].effect;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    Tile *tileset = tileCurrentTilesetInfo();
    if (!tileInfoLoaded)
        tileLoadInfoFromXml();

    if (tileset[tile].mask & MASK_ANIMATED)
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
    extern Context *c;
    if (c->opacity)
        return tileTestBit(tile, MASK_OPAQUE);
    else return 0;
}

unsigned char tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
