/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <libxml/xmlmemory.h>

#include "tile.h"

#include "context.h"
#include "creature.h"
#include "error.h"
#include "location.h"
#include "tilemap.h"
#include "tileset.h"
#include "xml.h"

/**
 * Loads tile information from the xml node 'node', if it
 * is a valid tile node.
 */
void Tile::loadProperties(Tile *tile, void *xmlNode) {
    xmlNodePtr node = (xmlNodePtr)xmlNode;

    static std::map<string, TileAnimationStyle> animStyles;
    if (!animStyles.size()) {
        animStyles["none"] = ANIM_NONE;
        animStyles["scroll"] = ANIM_SCROLL;
        animStyles["campfire"] = ANIM_CAMPFIRE;
        animStyles["cityflag"] = ANIM_CITYFLAG;
        animStyles["castleflag"] = ANIM_CASTLEFLAG;
        animStyles["shipflag"] = ANIM_SHIPFLAG;
        animStyles["lcbflag"] = ANIM_LCBFLAG;
        animStyles["frames"] = ANIM_FRAMES;
    }

    /* ignore 'text' nodes */
    if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tile") != 0)
        return;
            
    tile->name = xmlGetPropAsStr(node, "name"); /* get the name of the tile */    
    tile->frames = 1;

    if (xmlPropExists(node, "animation")) {
        string animation = xmlGetPropAsStr(node, "animation");
        if (animStyles.find(animation) == animStyles.end())
            errorWarning("Warning: animation style '%s' not found", animation.c_str());

        tile->animation = animStyles[animation]; /* get the animation style of the tile, if any */    
    }
    else tile->animation = ANIM_NONE;

    tile->opaque = xmlGetPropAsBool(node, "opaque"); /* see if the tile is opaque */

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
Tile *Tile::findByName(string name) {    
    unsigned int i;

    for (i = 0; i < Tileset::tiles.size(); i++) { 
        Tile *tile = Tileset::tiles[i];
        if (tile->name.empty())
            errorFatal("Error: not all tiles have a \"name\" attribute");
            
        if (strcasecmp(name.c_str(), tile->name.c_str()) == 0)
            return tile;
    }

    return NULL;
}

/**
 * Returns the tile at the corresponding index of the current tileset
 */ 
MapTile Tile::translate(int index, string tileMap) {
    TileIndexMap* im = TileMap::get(tileMap);    
    if (im) {        
        Tile *tile = Tile::findByName((*im)[index]);
        if (!tile)
            errorFatal("Error: the tile '%s' was not found in the tileset", (*im)[index].c_str());
        
        /* FIXME: is tile->index accurate? almost definately not */
        return MapTile(tile->id, index - tile->index);
    }
    return MapTile();
}

unsigned int Tile::getIndex(TileId id) {
    return Tileset::tiles[id]->index;
}

/**
 * MapTile Class Implementation
 */
unsigned int MapTile::getIndex() const {
    return Tile::getIndex(id) + frame;
}

Direction MapTile::getDirection() const {
    if (isShip() || isPirateShip())
        return (Direction) (frame + DIR_WEST);
    else if (isHorse())
        return (Direction) ((frame << 1) + DIR_WEST);
    else
        return DIR_WEST;        /* some random default */
}

bool MapTile::setDirection(Direction d) {
    bool changed = true;
    
    /* if we're already pointing the right direction, do nothing! */
    if (getDirection() == d)
        return false;

    if (isShip() || isPirateShip())
        frame = d - DIR_WEST;
    else if (isHorse())
        frame = d == DIR_WEST ? 0 : 1;
    else
        changed = false;

    return changed;
}

void MapTile::advanceFrame() {
    if (++frame >= Tileset::tiles[id]->frames)
        frame = 0;
}

void MapTile::reverseFrame() {
    if (frame-- == 0)
        frame = Tileset::tiles[id]->frames - 1;    
}

#define TESTBIT(against)     (Tileset::tiles[id]->rule->mask & (against))
#define TESTMOVEBIT(against) (Tileset::tiles[id]->rule->movementMask & (against))
#define GETRULE              (Tileset::tiles[id]->rule)

bool MapTile::canWalkOn(Direction d) const {
    return DIR_IN_MASK(d, Tileset::tiles[id]->rule->walkonDirs) ? true : false;
}

bool MapTile::canWalkOff(Direction d) const {    
    return DIR_IN_MASK(d, Tileset::tiles[id]->rule->walkoffDirs) ? true : false;
}

bool MapTile::canAttackOver() const {
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */
    return isWalkable() || isSwimable() || isSailable() || TESTBIT(MASK_ATTACKOVER);        
}

bool MapTile::canLandBalloon() const {
    return TESTBIT(MASK_CANLANDBALLOON);
}

bool MapTile::isReplacement() const {
    return TESTBIT(MASK_REPLACEMENT);
}

bool MapTile::isWalkable() const {    
    return GETRULE->walkonDirs > 0;
}

bool MapTile::isCreatureWalkable() const {
    return canWalkOn(DIR_ADVANCE) && !TESTMOVEBIT(MASK_CREATURE_UNWALKABLE);
}

bool MapTile::isSwimable() const {
    return TESTMOVEBIT(MASK_SWIMABLE);
}

bool MapTile::isSailable() const {
    return TESTMOVEBIT(MASK_SAILABLE);
}

bool MapTile::isWater() const {
    return (isSwimable() || isSailable());
}

bool MapTile::isFlyable() const {
    return !TESTMOVEBIT(MASK_UNFLYABLE);
}

bool MapTile::isDoor() const {
    return TESTBIT(MASK_DOOR);
}

bool MapTile::isLockedDoor() const {
    return TESTBIT(MASK_LOCKEDDOOR);
}

bool MapTile::isChest() const {
    return TESTBIT(MASK_CHEST);
}

bool MapTile::isShip() const {
    return TESTBIT(MASK_SHIP);
}

bool MapTile::isPirateShip() const {
    Tile *pirate = Tile::findByName("pirate_ship");
    if (id == pirate->id)
        return true;
    return false;
}

bool MapTile::isHorse() const {
    return TESTBIT(MASK_HORSE);
}

bool MapTile::isBalloon() const {
    return TESTBIT(MASK_BALLOON);
}

bool MapTile::canDispel() const {
    return TESTBIT(MASK_DISPEL);
}

bool MapTile::canTalkOver() const {
    return TESTBIT(MASK_TALKOVER);
}

TileSpeed MapTile::getSpeed() const {    
    return GETRULE->speed;
}

TileEffect MapTile::getEffect() const {
    return GETRULE->effect;
}

TileAnimationStyle MapTile::getAnimationStyle() const {
    return Tileset::tiles[id]->animation;
}

bool MapTile::isOpaque() const {
    extern Context *c;    

    if (c->opacity)
        return Tileset::tiles[id]->opaque ? 1 : 0;
    else return 0;
}


bool MapTile::canTalkOverTile(MapTile tile) {
    return tile.canTalkOver();
}
bool MapTile::canAttackOverTile(MapTile tile) {
    return tile.canAttackOver();
}

MapTile MapTile::tileForClass(int klass) {    
    return Tile::translate((klass * 2) + 0x20);
}

#undef TESTBIT
#undef TESTMOVEBIT
#undef GETRULE
