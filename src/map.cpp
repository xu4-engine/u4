/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "u4.h"

#include "map.h"

#include "annotation.h"
#include "context.h"
#include "creature.h"
#include "debug.h"
#include "direction.h"
#include "location.h"
#include "mapmgr.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "tile.h"
#include "utils.h"

/**
 * MapCoords Class Implementation
 */ 

bool MapCoords::operator==(const MapCoords &a) const {        
    return ((x == a.x) && (y == a.y) && (z == a.z)) ? true : false;        
}
bool MapCoords::operator!=(const MapCoords &a) const {
    return !operator==(a);
}    

MapCoords& MapCoords::wrap(const Map *map) {
    if (map && map->border_behavior == BORDER_WRAP) {
        while (x < 0)
            x += map->width;
        while (y < 0)
            y += map->height;
        while (x >= (int)map->width)
            x -= map->width;
        while (y >= (int)map->height)
            y -= map->height;
    }
    return *this;
}

MapCoords& MapCoords::putInBounds(const Map *map) {
    if (map) {
        if (x < 0)
            x = 0;
        if (x >= (int) map->width)
            x = map->width - 1;
        if (y < 0)
            y = 0;
        if (y >= (int) map->height)
            y = map->height - 1;
        if (z < 0)
            z = 0;
        if (z >= (int) map->levels)
            z = map->levels - 1;
    }
    return *this;
}

MapCoords& MapCoords::move(Direction d, const Map *map) {
    switch(d) {
    case DIR_NORTH: y--; break;
    case DIR_EAST: x++; break;
    case DIR_SOUTH: y++; break;
    case DIR_WEST: x--; break;
    default: break;
    }
    
    // Wrap the coordinates if necessary
    wrap(map);

    return *this;
}

MapCoords& MapCoords::move(int dx, int dy, const Map *map) {
    x += dx;
    y += dy;        
    
    // Wrap the coordinates if necessary
    wrap(map);

    return *this;
}

/**
 * Returns a mask of directions that indicate where one point is relative
 * to another.  For instance, if the object at (x, y) is
 * northeast of (c.x, c.y), then this function returns
 * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
 * This function also takes into account map boundaries and adjusts
 * itself accordingly. If the two coordinates are not on the same z-plane,
 * then this function return DIR_NONE.
 */
int MapCoords::getRelativeDirection(const MapCoords &c, const Map *map) const {
    int dx, dy, dirmask;        

    dirmask = DIR_NONE;
    if (z != c.z)
        return dirmask;
    
    /* adjust our coordinates to find the closest path */
    if (map && map->border_behavior == BORDER_WRAP) {
        MapCoords me = *this;            
        
        if (abs(me.x - c.x) > abs(me.x + map->width - c.x))
            me.x += map->width;
        else if (abs(me.x - c.x) > abs(me.x - map->width - c.x))
            me.x -= map->width;

        if (abs(me.y - c.y) > abs(me.y + map->width - c.y))
            me.y += map->height;
        else if (abs(me.y - c.y) > abs(me.y - map->width - c.y))
            me.y -= map->height;

        dx = me.x - c.x;
        dy = me.y - c.y;
    }
    else {        
        dx = x - c.x;
        dy = y - c.y;
    }

    /* add x directions that lead towards to_x to the mask */
    if (dx < 0)         dirmask |= MASK_DIR(DIR_EAST);
    else if (dx > 0)    dirmask |= MASK_DIR(DIR_WEST);

    /* add y directions that lead towards to_y to the mask */
    if (dy < 0)         dirmask |= MASK_DIR(DIR_SOUTH);
    else if (dy > 0)    dirmask |= MASK_DIR(DIR_NORTH);

    /* return the result */
    return dirmask;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 * This function also takes into account map boundaries and adjusts
 * itself accordingly, provided the 'map' parameter is passed
 */
Direction MapCoords::pathTo(const MapCoords &c, int valid_directions, bool towards, const Map *map) const {
    int directionsToObject;

    /* find the directions that lead [to/away from] our target */
    directionsToObject = towards ? getRelativeDirection(c, map) : ~getRelativeDirection(c, map);

    /* make sure we eliminate impossible options */
    directionsToObject &= valid_directions;

    /* get the new direction to move */
    if (directionsToObject > DIR_NONE)
        return dirRandomDir(directionsToObject);

    /* there are no valid directions that lead to our target, just move wherever we can! */
    else return dirRandomDir(valid_directions);
}

/**
 * Finds the appropriate direction to travel to move away from one point
 */
Direction MapCoords::pathAway(const MapCoords &c, int valid_directions) const {
    return pathTo(c, valid_directions, false);
}

/**
 * Finds the movement distance (not using diagonals) from point a to point b
 * on a map, taking into account map boundaries and such.  If the two coords
 * are not on the same z-plane, then this function returns -1;
 */
int MapCoords::movementDistance(const MapCoords &c, const Map *map) const {
    int dirmask = DIR_NONE;
    int dist = 0;
    MapCoords me = *this;

    if (z != c.z)
        return -1;

    /* get the direction(s) to the coordinates */
    dirmask = getRelativeDirection(c, map);

    while ((me.x != c.x) || (me.y != c.y))
    {
        if (me.x != c.x) {
            if (dirmask & MASK_DIR_WEST)
                me.move(DIR_WEST, map);
            else me.move(DIR_EAST, map);

            dist++;
        }
        if (me.y != c.y) {
            if (dirmask & MASK_DIR_NORTH)
                me.move(DIR_NORTH, map);
            else me.move(DIR_SOUTH, map);

            dist++;
        }            
    }

    return dist;
}

/**
 * Finds the distance (using diagonals) from point a to point b on a map
 * If the two coordinates are not on the same z-plane, then this function
 * returns -1. This function also takes into account map boundaries.
 */ 
int MapCoords::distance(const MapCoords &c, const Map *map) const {
    int dist = movementDistance(c, map);
    if (dist <= 0)
        return dist;

    /* calculate how many fewer movements there would have been */
    dist -= abs(x - c.x) < abs(y - c.y) ? abs(x - c.x) : abs(y - c.y);

    return dist;
}

/**
 * Map Class Implementation
 */ 

Map::Map() {}
Map::Map(MapId id) {
    *this = *mapMgrGetById(id);
}
Map::~Map() {}

string Map::getName() {
    return fname;
}

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Object *Map::objectAt(MapCoords coords) {
    /* FIXME: return a list instead of one object */
    ObjectDeque::const_iterator i;        
    Object *objAt = NULL;    

    for(i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;
        
        if (obj->getCoords() == coords) {
            /* get the most visible object */
            if (objAt && (objAt->getType() == OBJECT_UNKNOWN) && (obj->getType() != OBJECT_UNKNOWN))
                objAt = obj;
            /* give priority to objects that have the focus */
            else if (objAt && (!objAt->hasFocus()) && (obj->hasFocus()))
                objAt = obj;
            else if (!objAt)
                objAt = obj;
        }
    }
    return objAt;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *Map::portalAt(MapCoords coords, int actionFlags) {
    PortalList::const_iterator i;    

    for(i = portals.begin(); i != portals.end(); i++) {
        if (((*i)->coords == coords) &&
            ((*i)->trigger_action & actionFlags))
            return *i;
    }
    return NULL;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
MapTile Map::getTileFromData(Coords coords) {
    int index;

    if (MAP_IS_OOB(this, coords))
        return MapTile(0);

    index = coords.x + (coords.y * width) + (width * height * coords.z);    
    return data[index];
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
MapTile Map::tileAt(Coords coords, int withObjects) {    
    /* FIXME: this should return a list of tiles, with the most visible at the front */
    MapTile tile;
    AnnotationList a = annotations->allAt(coords);
    Object *obj = objectAt(coords);    
 
    tile = getTileFromData(coords);
    /* FIXME: this only returns the first valid annotation it can find */
    if (a.size() > 0) {
        AnnotationList::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (!i->isVisualOnly())            
                return i->getTile();
        }
    }    
    
    if ((withObjects == WITH_OBJECTS) && obj)
        tile = obj->getTile();
    else if ((withObjects == WITH_GROUND_OBJECTS) && obj && tileIsWalkable(obj->getTile()))
        tile = obj->getTile();
    
    return tile;
}

/**
 * Returns true if the given map is the world map
 */
bool Map::isWorldMap() {
    return type == MAPTYPE_WORLD;
}

/**
 * Adds a creature object to the given map
 */
Creature *Map::addCreature(const Creature *creature, Coords coords) {
    Creature *m = new Creature;
    
    /* make a copy of the creature before placing it */
    *m = *creature;

    m->setInitialHp();
    m->setStatus(STAT_GOOD);    
    m->setCoords(coords);
	m->setMap(this);
    
    /* initialize the creature before placing it */
    if (m->wanders())
        m->setMovementBehavior(MOVEMENT_WANDER);
    else if (m->isStationary())
        m->setMovementBehavior(MOVEMENT_FIXED);
    else m->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);

    /* hide camouflaged creatures from view during combat */
    if (m->camouflages() && (type == MAPTYPE_COMBAT))
        m->setVisible(false);
    
    /* place the creature on the map */
    objects.push_back(m);
    return m;
}

/**
 * Adds an object to the given map
 */
Object *Map::addObject(MapTile tile, MapTile prevtile, Coords coords) {
    Object *obj = new Object;

    obj->setTile(tile);
    obj->setPrevTile(prevtile);
    obj->setCoords(coords);    
    obj->setPrevCoords(coords);
	obj->setMap(this);
    
    objects.push_front(obj);    

    return obj;
}

/**
 * Removes an object from the map
 */ 

// This function should only be used when not iterating through an
// ObjectDeque, as the iterator will be invalidated and the
// results will be unpredictable.  Instead, use the function
// below.
void Map::removeObject(const Object *rem) {
    ObjectDeque::iterator i;
    for (i = objects.begin(); i != objects.end(); i++) {
        if (*i == rem) {
            /* Party members persist through different maps, so don't delete them! */
            if (!isPartyMember(*i))
                delete (*i);
            objects.erase(i);
            return;
        }
    }
}

ObjectDeque::iterator Map::removeObject(ObjectDeque::iterator rem) {
    /* Party members persist through different maps, so don't delete them! */
    if (!isPartyMember(*rem))
        delete (*rem);
    return objects.erase(rem);
}

/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a creature attacking.
 * Also performs special creature actions and creature effects.
 */
Creature *Map::moveObjects(MapCoords avatar) {        
    ObjectDeque::iterator i;
    Creature *attacker = NULL;

    for (i = objects.begin(); i != objects.end(); i++) {
        Creature *m = dynamic_cast<Creature*>(*i);

        if (m) {
            /* check if the object is an attacking creature and not
               just a normal, docile person in town or an inanimate object */
            if ((m->getType() == OBJECT_PERSON && m->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR) ||
                (m->getType() == OBJECT_CREATURE && m->willAttack())) {
                MapCoords o_coords = m->getCoords();
            
                /* don't move objects that aren't on the same level as us */
                if (o_coords.z != avatar.z)
                    continue;
            
                if (o_coords.movementDistance(avatar, this) <= 1) {
                    attacker = m;
                    continue;
                }
            }

            /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
            if (!m->specialAction())
                moveObject(this, m, avatar);

            /* Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
            m->specialEffect();
        }
    }

    return attacker;
}

/**
 * Animates the objects on the given map
 */
void Map::animateObjects() {
    ObjectDeque::iterator i;
    
    for (i = objects.begin(); i != objects.end(); i++) {
        if ((*i)->isAnimated() && xu4_random(2))
            (*i)->advanceFrame();
    }
}

/**
 * Resets object animations to a value that is acceptable for
 * savegame compatibility with u4dos.
 */
void Map::resetObjectAnimations() {
    ObjectDeque::iterator i;
    
    for (i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;
        
        if (obj->getType() == OBJECT_CREATURE)
            obj->setPrevTile(creatures.getByTile(obj->getTile())->getTile());        
    }
}

/**
 * Removes all objects from the given map
 */
void Map::clearObjects() {
    objects.clear();    
}

/**
 * Returns the number of creatures on the given map
 */
int Map::getNumberOfCreatures() {
    ObjectDeque::const_iterator i;
    int n = 0;

    for (i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;

        if (obj->getType() == OBJECT_CREATURE)
            n++;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int Map::getValidMoves(MapCoords from, MapTile transport) {
    int retval;
    Direction d;
    MapTile tile, prev_tile;
    Object *obj;    
    const Creature *m, *to_m;
    int ontoAvatar, ontoCreature;    
    MapCoords coords = from;
    bool isAvatar = (c->location->coords == coords);

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        coords = from;
        ontoAvatar = 0;
        ontoCreature = 0;

        // Move the coordinates in the current direction and test it
        coords.move(d, this);
        
        // you can always walk off the edge of the map
        if (MAP_IS_OOB(this, coords)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        obj = objectAt(coords);

        // see if it's trying to move onto the avatar
        if ((flags & SHOW_AVATAR) && (coords == c->location->coords))
            ontoAvatar = 1;
        
        // see if it's trying to move onto a person or creature
        else if (obj && (obj->getType() != OBJECT_UNKNOWN))                 
            ontoCreature = 1;
            
        // get the destination tile
        if (ontoAvatar)
            tile = (MapTile)c->saveGame->transport;
        else if (ontoCreature)
            tile = obj->getTile();
        else 
            tile = tileAt(coords, WITH_OBJECTS);

        prev_tile = tileAt(from, WITHOUT_OBJECTS);

        // get the creature object, if it exists (the one that's moving)
        m = creatures.getByTile(transport);        
        // get the other creature object, if it exists (the one that's being moved onto)        
        to_m = dynamic_cast<Creature*>(obj);

        // move on if unable to move onto the avatar or another creature
        if (m && !isAvatar) { // some creatures/persons have the same tile as the avatar, so we have to adjust
            if ((ontoAvatar && !m->canMoveOntoPlayer()) ||
                (ontoCreature && !m->canMoveOntoCreatures() && !to_m->canMoveOnto()))
                continue;
        }
        // this really only happens with the avatar
        else if (ontoCreature && to_m && to_m->canMoveOnto()) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        // avatar movement
        if (isAvatar) {
            // if the transport is a ship, check sailable
            if (tileIsShip(transport) && tileIsSailable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
            // if it is a balloon, check flyable
            else if (tileIsBalloon(transport) && tileIsFlyable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);        
            // avatar or horseback: check walkable
            else if (transport == AVATAR_TILE || tileIsHorse(transport)) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
        }
        
        // creature movement
        else if (m) {
            // flying creatures
            if (tileIsFlyable(tile) && m->flies()) {  
                // FIXME: flying creatures behave differently on the world map?
                if (isWorldMap())
                    retval = DIR_ADD_TO_MASK(d, retval);
                else if (tileIsWalkable(tile) || tileIsSwimable(tile) || tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // swimming creatures and sailing creatures
            else if (tileIsSwimable(tile) || tileIsSailable(tile)) {
                if (m->swims() && tileIsSwimable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (m->sails() && tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // ghosts and other incorporeal creatures
            else if (m->isIncorporeal()) {
                // can move anywhere but onto water, unless of course the creature can swim
                if (!(tileIsSwimable(tile) || tileIsSailable(tile)))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // walking creatures
            else if (m->walks()) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d) &&
                    tileIsCreatureWalkable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }            
        }
    }

    return retval;
}

bool Map::move(Object *obj, Direction d) {
    MapCoords new_coords = obj->getCoords();
    if (new_coords.move(d) != obj->getCoords()) {
        obj->setCoords(new_coords);
        return true;
    }
    return false;
}
