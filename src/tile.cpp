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
#include "utils.h"
#include "xml.h"

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

    /* get the animation for the tile, if one is specified */
    if (xmlPropExists(node, "animation")) {
        extern TileAnimSet *tileanims;
        string animation = xmlGetPropAsStr(node, "animation");

        tile->anim = tileanims->getByName(animation);
        if (tile->anim == NULL)
            errorWarning("Warning: animation style '%s' not found", animation.c_str());        
    }
    else tile->anim = NULL;    

    /* see if the tile is opaque */
    tile->opaque = xmlGetPropAsBool(node, "opaque"); 

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

    /* if the tile looks exactly like another tile, record it */
    if (xmlPropExists(node, "looks_like"))
        tile->looks_like = xmlGetPropAsStr(node, "looks_like");

    /* get the index, if it is provided.  Otherwise, it is implied */
    if (xmlPropExists(node, "index"))
        tile->index = xmlGetPropAsInt(node, "index");
    else tile->index = -1;
}

/**
 * Returns the tile at the corresponding index of the current tileset
 */ 
MapTile Tile::translate(int index, string tileMap) {    
    TileIndexMap* im = TileMap::get(tileMap);
    if (im)
        return (*im)[index];         

    return MapTile();
}

unsigned int Tile::getIndex(TileId id) {
    return c && c->location ? c->location->tileset->get(id)->index : Tileset::get("base")->get(id)->index;    
}

Image *Tile::getImage() {
    return image;
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
    Tileset *t = Tileset::get();
    if (++frame >= t->get(id)->frames)
        frame = 0;
}

void MapTile::reverseFrame() {
    Tileset *t = Tileset::get();
    if (frame-- == 0)
        frame = t->get(id)->frames - 1;    
}

#define TESTBIT(against)     (t->get(id)->rule->mask & (against))
#define TESTMOVEBIT(against) (t->get(id)->rule->movementMask & (against))
#define GETRULE              (t->get(id)->rule)

bool MapTile::canWalkOn(Direction d) const {
    Tileset *t = Tileset::get();
    return DIR_IN_MASK(d, t->get(id)->rule->walkonDirs) ? true : false;
}

bool MapTile::canWalkOff(Direction d) const {    
    Tileset *t = Tileset::get();
    return DIR_IN_MASK(d, t->get(id)->rule->walkoffDirs) ? true : false;
}

bool MapTile::canAttackOver() const {
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */
    Tileset *t = Tileset::get();
    return isWalkable() || isSwimable() || isSailable() || TESTBIT(MASK_ATTACKOVER);        
}

bool MapTile::canLandBalloon() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_CANLANDBALLOON);
}

bool MapTile::isReplacement() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_REPLACEMENT);
}

bool MapTile::isWalkable() const {    
    Tileset *t = Tileset::get();
    return GETRULE->walkonDirs > 0;
}

bool MapTile::isCreatureWalkable() const {
    Tileset *t = Tileset::get();
    return canWalkOn(DIR_ADVANCE) && !TESTMOVEBIT(MASK_CREATURE_UNWALKABLE);
}

bool MapTile::isSwimable() const {
    Tileset *t = Tileset::get();
    return TESTMOVEBIT(MASK_SWIMABLE);
}

bool MapTile::isSailable() const {
    Tileset *t = Tileset::get();
    return TESTMOVEBIT(MASK_SAILABLE);
}

bool MapTile::isWater() const {
    return (isSwimable() || isSailable());
}

bool MapTile::isFlyable() const {
    Tileset *t = Tileset::get();
    return !TESTMOVEBIT(MASK_UNFLYABLE);
}

bool MapTile::isDoor() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_DOOR);
}

bool MapTile::isLockedDoor() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_LOCKEDDOOR);
}

bool MapTile::isChest() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_CHEST);
}

bool MapTile::isShip() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_SHIP);
}

bool MapTile::isPirateShip() const {
    Tile *pirate = Tileset::findTileByName("pirate_ship");
    if (id == pirate->id)
        return true;
    return false;
}

bool MapTile::isHorse() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_HORSE);
}

bool MapTile::isBalloon() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_BALLOON);
}

bool MapTile::canDispel() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_DISPEL);
}

bool MapTile::canTalkOver() const {
    Tileset *t = Tileset::get();
    return TESTBIT(MASK_TALKOVER);
}

TileSpeed MapTile::getSpeed() const { 
    Tileset *t = Tileset::get();
    return GETRULE->speed;
}

TileEffect MapTile::getEffect() const {
    Tileset *t = Tileset::get();
    return GETRULE->effect;
}

TileAnimationStyle MapTile::getAnimationStyle() const {
    Tileset *t = Tileset::get();
    TileAnim* anim = t->get(id)->anim;
    if (anim)
        return anim->name == "framed" ? ANIM_FRAMES : ANIM_NONE;
    else return ANIM_NONE;
}

bool MapTile::isOpaque() const {
    extern Context *c;    
    Tileset *t = Tileset::get();

    if (c->opacity)
        return t->get(id)->opaque ? 1 : 0;
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
