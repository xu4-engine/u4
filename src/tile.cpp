/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <libxml/xmlmemory.h>

#include "tile.h"

#include "context.h"
#include "error.h"
#include "location.h"
#include "creature.h"
#include "tileset.h"
#include "xml.h"

/**
 * 
 */
Tileset *tilesetGetCurrent() {
    return (c && c->location) ? c->location->tileset : Tileset::get(TILESET_BASE);
}

/**
 * Loads tile information from the xml node 'node', if it
 * is a valid tile node.
 */
void Tile::loadProperties(Tile *tile, void *xmlNode) {
    xmlNodePtr node = (xmlNodePtr)xmlNode;

    /* ignore 'text' nodes */        
    if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tile") != 0)
        return;
            
    tile->name = xmlGetPropAsStr(node, "name"); /* get the name of the tile */    
    tile->frames = 1;
    tile->animated = xmlGetPropAsBool(node, "animated"); /* see if the tile is animated */
    tile->opaque = xmlGetPropAsBool(node, "opaque"); /* see if the tile is opaque */

    /* get the tile to display for the current tile */
    if (xmlPropExists(node, "displayTile"))
        tile->displayTile = xmlGetPropAsInt(node, "displayTile");    

    /* find the rule that applies to the current tile, if there is one.
       if there is no rule specified, it defaults to the "default" rule */
    if (xmlPropExists(node, "rule")) {
        tile->rule = TileRule::findByName(xmlGetPropAsStr(node, "rule"));
        if (tile->rule == NULL)
            tile->rule = TileRule::findByName("default");
    }
    else tile->rule = TileRule::findByName("default");

    /* get the number of frames the tile has */    
    if (xmlPropExists(node, "frames"))
        tile->frames = xmlGetPropAsInt(node, "frames");    
}

/**
 * Returns the tile with the given name in the tileset
 */ 
Tile *Tile::findByName(string name, Tileset *t) {
    Tileset *tileset = t ? t : tilesetGetCurrent();
    int i;

    for (i = 0; i < tileset->tiles.size(); i++) { 
        Tile *tile = tileset->tiles[i];
        if (tile->name.empty())
            errorFatal("Error: not all tiles have a \"name\" attribute");
            
        if (strcasecmp(name.c_str(), tile->name.c_str()) == 0)
            return tile;
    }

    return NULL;
}

Tile *Tile::getTile(int index) {
    Tileset *tileset = tilesetGetCurrent();
    return tileset->tiles[tileset->indexMap[index]];
}

/**
 * Returns the tile at the corresponding index of the current tileset
 */ 
MapTile Tile::getMapTile(int index) {
    Tileset *tileset = tilesetGetCurrent();    
    return MapTile(tileset->indexMap[index]);    
}

bool tileTestBit(MapTile tile, unsigned short mask) {    
    return (tilesetGetCurrent()->tiles[tile]->rule->mask & mask) != 0;
}

bool tileTestMovementBit(MapTile tile, unsigned short mask) {    
    return (tilesetGetCurrent()->tiles[tile]->rule->movementMask & mask) != 0;
}

bool tileCanWalkOn(MapTile tile, Direction d) {    
    return DIR_IN_MASK(d, tilesetGetCurrent()->tiles[tile]->rule->walkonDirs) ? true : false;
}

bool tileCanWalkOff(MapTile tile, Direction d) {    
    return DIR_IN_MASK(d, tilesetGetCurrent()->tiles[tile]->rule->walkoffDirs) ? true : false;
}

bool tileCanAttackOver(MapTile tile) {    
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */
    return tileIsWalkable(tile) || tileIsSwimable(tile) || tileIsSailable(tile) ||       
        tileTestBit(tile, MASK_ATTACKOVER);
}

bool tileCanLandBalloon(MapTile tile) {
    return tileTestBit(tile, MASK_CANLANDBALLOON);
}

bool tileIsReplacement(MapTile tile) {
    return tileTestBit(tile, MASK_REPLACEMENT);
}

bool tileIsWalkable(MapTile tile) {    
    return tilesetGetCurrent()->tiles[tile]->rule->walkonDirs > 0;
}

bool tileIsCreatureWalkable(MapTile tile) {
    return tileCanWalkOn(tile, DIR_ADVANCE) && !tileTestMovementBit(tile, MASK_CREATURE_UNWALKABLE);
}

bool tileIsSwimable(MapTile tile) {
    return tileTestMovementBit(tile, MASK_SWIMABLE);
}

bool tileIsSailable(MapTile tile) {
    return tileTestMovementBit(tile, MASK_SAILABLE);
}

bool tileIsWater(MapTile tile) {
    return (tileIsSwimable(tile) | tileIsSailable(tile));
}

bool tileIsFlyable(MapTile tile) {
    return !tileTestMovementBit(tile, MASK_UNFLYABLE);
}

bool tileIsDoor(MapTile tile) {
    return tileTestBit(tile, MASK_DOOR);
}

bool tileIsLockedDoor(MapTile tile) {
    return tileTestBit(tile, MASK_LOCKEDDOOR);
}

bool tileIsChest(MapTile tile) {
    return tileTestBit(tile, MASK_CHEST);
}

MapTile tileGetChestBase() {
    return Tile::findByName("chest")->index;
}

bool tileIsShip(MapTile tile) {
    return tileTestBit(tile, MASK_SHIP);
}

MapTile tileGetShipBase() {
    return Tile::findByName("ship")->index;
}

bool tileIsPirateShip(MapTile tile) {
    Tile *pirate = Tile::getTile(PIRATE_TILE);
    if (tile >= pirate->id && tile < pirate->id + pirate->frames)
        return true;
    return false;
}

bool tileIsHorse(MapTile tile) {
    return tileTestBit(tile, MASK_HORSE);
}

MapTile tileGetHorseBase() {
    return Tile::findByName("horse")->index;
}

bool tileIsBalloon(MapTile tile) {
    return tileTestBit(tile, MASK_BALLOON);
}

MapTile tileGetBalloonBase() {
    return Tile::findByName("balloon")->index;
}

bool tileCanDispel(MapTile tile) {
    return tileTestBit(tile, MASK_DISPEL);
}

Direction tileGetDirection(MapTile tile) {
    if (tileIsShip(tile))
        return (Direction) (tile - Tile::findByName("ship")->index + DIR_WEST);
    if (tileIsPirateShip(tile))
        return (Direction) (tile - PIRATE_TILE + DIR_WEST);
    else if (tileIsHorse(tile))
        return tile == Tile::findByName("horse")->index ? DIR_WEST : DIR_EAST;
    else
        return DIR_WEST;        /* some random default */
}

bool tileSetDirection(MapTile *tile, Direction dir) {
    bool changed_direction = true;
    int oldTile = *tile;

    /* Make sure we even have a direction */
    if (dir <= DIR_NONE)
        return false;

    if (tileIsShip(*tile))
        *tile = Tile::findByName("ship")->index + dir - DIR_WEST;
    else if (tileIsPirateShip(*tile))
        *tile = PIRATE_TILE + dir - DIR_WEST;
    else if (tileIsHorse(*tile))
        *tile = (dir == DIR_WEST ? Tile::findByName("horse")->index : Tile::findByName("horse")->index + 1);
    else   
        changed_direction = false;

    if (oldTile == *tile)
        changed_direction = false;

    return changed_direction;
}

bool tileCanTalkOver(MapTile tile) {
    return tileTestBit(tile, MASK_TALKOVER);
}

TileSpeed tileGetSpeed(MapTile tile) {    
    return tilesetGetCurrent()->tiles[tile]->rule->speed;
}

TileEffect tileGetEffect(MapTile tile) {    
    return tilesetGetCurrent()->tiles[tile]->rule->effect;
}

TileAnimationStyle tileGetAnimationStyle(MapTile tile) {   
    if (tilesetGetCurrent()->tiles[tile]->animated)
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

void tileAdvanceFrame(MapTile *tile) {
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

bool tileIsOpaque(MapTile tile) {
    extern Context *c;    

    if (c->opacity)
        return tilesetGetCurrent()->tiles[tile]->opaque ? 1 : 0;
    else return 0;
}

MapTile tileForClass(int klass) {
    return (klass * 2) + 0x20;
}
