/*
 * $Id$
 */

#include <stddef.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "tile.h"

#include "context.h"
#include "error.h"
#include "monster.h"
#include "tileset.h"
#include "xml.h"

/**
 * 
 */
Tile *tileCurrentTilesetInfo() {
    return (c && c->location) ? c->location->tileset->tiles : tilesetGetByType(TILESET_BASE)->tiles;
}

/**
 * Loads tile information from the xml node 'node', if it
 * is a valid tile node.
 */
int tileLoadTileInfo(Tile** tiles, int index, void *xmlNode) {    
    int i;
    Tile tile, *tilePtr;
    xmlNodePtr node = (xmlNodePtr)xmlNode;

    /* ignore 'text' nodes */        
    if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tile") != 0)
        return 1;
            
    tile.name = xmlGetPropAsStr(node, "name"); /* get the name of the tile */
    tile.index = index; /* get the index of the tile */
    tile.frames = 1;
    tile.animated = xmlGetPropAsBool(node, "animated"); /* see if the tile is animated */
    tile.opaque = xmlGetPropAsBool(node, "opaque"); /* see if the tile is opaque */

    /* get the tile to display for the current tile */
    if (xmlPropExists(node, "displayTile"))
        tile.displayTile = xmlGetPropAsInt(node, "displayTile");
    else
        tile.displayTile = index; /* itself */    

    /* find the rule that applies to the current tile, if there is one.
       if there is no rule specified, it defaults to the "default" rule */
    if (xmlPropExists(node, "rule")) {
        tile.rule = tilesetFindRuleByName(xmlGetPropAsStr(node, "rule"));
        if (tile.rule == NULL)
            tile.rule = tilesetFindRuleByName("default");
    }
    else tile.rule = tilesetFindRuleByName("default");

    /* for each frame of the tile, duplicate our values */    
    if (xmlPropExists(node, "frames"))
        tile.frames = xmlGetPropAsInt(node, "frames");
    
    tilePtr = (*tiles) + index;
    for (i = 0; i < tile.frames; i++) {        
        memcpy(tilePtr + i, &tile, sizeof(Tile));
        (tilePtr + i)->index += i; /* fix the index */        
    }
    
    return 1;
}

Tile *tileFindByName(const char *name) {
    /* FIXME: rewrite for new system */
    Tile *tiles = tileCurrentTilesetInfo();
    int i;

    if (!name)
        return NULL;

    for (i = 0; i < 256; i++) {
        if (tiles[i].name == NULL)
            errorFatal("Error: not all tiles have a \"name\" attribute");
            
        if (strcasecmp(name, tiles[i].name) == 0)
            return &tiles[i];
    }

    return NULL;
}

int tileTestBit(unsigned char tile, unsigned short mask) {
    Tile *tiles = tileCurrentTilesetInfo();
    return (tiles[tile].rule->mask & mask) != 0;
}

int tileTestMovementBit(unsigned char tile, unsigned short mask) {
    Tile *tiles = tileCurrentTilesetInfo();
    return (tiles[tile].rule->movementMask & mask) != 0;
}

int tileCanWalkOn(unsigned char tile, Direction d) {
    Tile *tiles = tileCurrentTilesetInfo();
    return DIR_IN_MASK(d, tiles[tile].rule->walkonDirs);
}

int tileCanWalkOff(unsigned char tile, Direction d) {
    Tile *tiles = tileCurrentTilesetInfo();
    return DIR_IN_MASK(d, tiles[tile].rule->walkoffDirs);
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
    Tile *tiles = tileCurrentTilesetInfo();
    return tiles[tile].rule->walkonDirs > 0;
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
    return tileFindByName("chest")->index;
}

int tileIsShip(unsigned char tile) {
    return tileTestBit(tile, MASK_SHIP);
}

unsigned char tileGetShipBase() {
    return tileFindByName("ship")->index;
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
    return tileFindByName("horse")->index;
}

int tileIsBalloon(unsigned char tile) {
    return tileTestBit(tile, MASK_BALLOON);
}

unsigned char tileGetBalloonBase() {
    return tileFindByName("balloon")->index;
}

int tileCanDispel(unsigned char tile) {
    return tileTestBit(tile, MASK_DISPEL);
}

Direction tileGetDirection(unsigned char tile) {
    if (tileIsShip(tile))
        return (Direction) (tile - tileFindByName("ship")->index + DIR_WEST);
    if (tileIsPirateShip(tile))
        return (Direction) (tile - PIRATE_TILE + DIR_WEST);
    else if (tileIsHorse(tile))
        return tile == tileFindByName("horse")->index ? DIR_WEST : DIR_EAST;
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
        *tile = tileFindByName("ship")->index + dir - DIR_WEST;
    else if (tileIsPirateShip(*tile))
        *tile = PIRATE_TILE + dir - DIR_WEST;
    else if (tileIsHorse(*tile))
        *tile = (dir == DIR_WEST ? tileFindByName("horse")->index : tileFindByName("horse")->index + 1);
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
    Tile *tiles = tileCurrentTilesetInfo();
    return tiles[tile].rule->speed;
}

TileEffect tileGetEffect(unsigned char tile) {
    Tile *tiles = tileCurrentTilesetInfo();
    return tiles[tile].rule->effect;
}

TileAnimationStyle tileGetAnimationStyle(unsigned char tile) {
    Tile *tiles = tileCurrentTilesetInfo();
   
    if (tiles[tile].animated)
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
    Tile *tiles = tileCurrentTilesetInfo();

    if (c->opacity)
        return tiles[tile].opaque ? 1 : 0;
    else return 0;
}

unsigned char tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
