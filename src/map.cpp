/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include "u4.h"

#include "map.h"

#include "annotation.h"
#include "context.h"
#include "debug.h"
#include "direction.h"
#include "location.h"
#include "monster.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "savegame.h"
#include "tile.h"
#include "utils.h"

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Object *mapObjectAt(const Map *map, MapCoords coords) {
    Object *obj;
    Object *objAt = NULL;

    for(obj = map->objects; obj; obj = obj->next) {
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
 * Returns the person object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
const Person *mapPersonAt(const Map *map, MapCoords coords) {
    Object *obj;

    obj = mapObjectAt(map, coords);
    if (obj && obj->getType() == OBJECT_PERSON)
        return obj->person;
    else
        return NULL;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *mapPortalAt(const Map *map, MapCoords coords, int actionFlags) {
    PortalList::const_iterator i;    

    for(i = map->portals.begin(); i != map->portals.end(); i++) {
        if (((*i)->coords == coords) &&
            ((*i)->trigger_action & actionFlags))
            return *i;
    }
    return NULL;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
MapTile mapGetTileFromData(const Map *map, MapCoords coords) {
    int index;

    if (MAP_IS_OOB(map, coords))
        return MapTile(0);

    index = coords.x + (coords.y * map->width) + (map->width * map->height * coords.z);    
    return map->data[index];
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
MapTile mapTileAt(const Map *map, MapCoords coords, int withObjects) {    
    MapTile tile;    
    AnnotationList a = c->location->map->annotations->allAt(coords);
    Object *obj = mapObjectAt(map, coords);    
 
    tile = mapGetTileFromData(map, coords);
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
int mapIsWorldMap(const Map *map) {
    return map->type == MAPTYPE_WORLD;
}

/**
 * Adds a person object to the given map
 */
Object *mapAddPersonObject(Map *map, const Person *person) {
    Object *obj = mapAddObject(map, person->tile0, person->tile1, MapCoords(person->startx, person->starty, person->startz));

    obj->setMovementBehavior(person->movement_behavior);
    obj->person = person;
    obj->setType(OBJECT_PERSON);

    return obj;
}

/**
 * Adds a monster object to the given map
 */
Object *mapAddMonsterObject(Map *map, const Monster *monster, MapCoords coords) {
    Object *obj = mapAddObject(map, monster->tile, monster->tile, coords);

    if (monsterWanders(monster))
        obj->setMovementBehavior(MOVEMENT_WANDER);
    else if (monsterIsStationary(monster))
        obj->setMovementBehavior(MOVEMENT_FIXED);
    else obj->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);

    /* hide camouflaged monsters from view during combat */
    if (monsterCamouflages(monster) && (map->type == MAPTYPE_COMBAT))
        obj->setVisible(false);

    obj->monster = monster;
    obj->setType(OBJECT_MONSTER);

    return obj;
}

/**
 * Adds an object to the given map
 */
Object *mapAddObject(Map *map, MapTile tile, MapTile prevtile, MapCoords coords) {
    Object *obj = new Object;

    obj->setTile(tile);
    obj->setPrevTile(prevtile);
    obj->setCoords(coords);    
    obj->setPrevCoords(coords);
    obj->next = map->objects;

    map->objects = obj;

    return obj;
}

/**
 * Removes the object 'rem' from the given map
 */
void mapRemoveObject(Map *map, Object *rem) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if (obj == rem) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
 
            /* free the memory used by a non-standard person object */
            if (obj->getType() == OBJECT_PERSON && !obj->person->permanent)
                delete (Person *)obj->person;

            delete obj;
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

/**
 * Removes the given Person object from the given map
 */
void mapRemovePerson(Map *map, const Person *person) {
    Object *obj = map->objects, *prev;

    prev = NULL;
    while (obj) {
        if ((obj->getType() == OBJECT_PERSON) && (obj->person == person)) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
            delete obj;
            return;
        }
        prev = obj;
        obj = obj->next;
    }
}

/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a monster attacking.
 * Also performs special monster actions and monster effects.
 */
Object *mapMoveObjects(Map *map, MapCoords avatar) {        
    Object *obj = map->objects, *attacker = NULL;        

    for (obj = map->objects; obj; obj = obj->next) {                
        
        /* check if the object is an attacking monster and not
           just a normal, docile person in town or an inanimate object */
        if ((obj->getType() != OBJECT_UNKNOWN) && 
           ((obj->getType() != OBJECT_MONSTER) || monsterWillAttack(obj->monster)) &&
           ((obj->getType() != OBJECT_PERSON) || (obj->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR))) {

            MapCoords o_coords = obj->getCoords();
            
            /* don't move objects that aren't on the same level as us */
            if (o_coords.z != avatar.z)
                continue;
            
            if (o_coords.movementDistance(avatar, c->location->map) == 1) {
                attacker = obj;
                continue;
            }
        }

        /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
        monsterSpecialAction(obj);

        /* Now, move the object according to its movement behavior */
        moveObject(map, obj, avatar);

        /* Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
        monsterSpecialEffect(obj);
    }

    return attacker;
}

/**
 * Animates the objects on the given map
 */
void mapAnimateObjects(Map *map) {
    Object *obj = map->objects;

    while (obj) {
        if (obj->isAnimated() && xu4_random(2))
            obj->advanceFrame();

        obj = obj->next;
    }
}

/**
 * Resets object animations to a value that is acceptable for
 * savegame compatibility with u4dos.
 */
void mapResetObjectAnimations(Map *map) {
    Object *obj = map->objects;        

    while (obj) {
        if (obj->getType() == OBJECT_PERSON) {
            obj->setPrevTile(obj->person->tile0);
            obj->setTile(obj->person->tile0);
        } else if (obj->getType() == OBJECT_MONSTER)
            obj->setPrevTile(obj->monster->tile);        

        obj = obj->next;
    }
}

/**
 * Removes all objects from the given map
 */
void mapClearObjects(Map *map) {
    while (map->objects)
        mapRemoveObject(map, map->objects);
    map->objects = NULL;
}

/**
 * Returns the number of monsters on the given map
 */
int mapNumberOfMonsters(const Map *map) {
    Object *obj = map->objects;
    int n;

    n = 0;
    while (obj) {
        if (obj->getType() == OBJECT_MONSTER)
            n++;

        obj = obj->next;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int mapGetValidMoves(const Map *map, MapCoords from, MapTile transport) {
    int retval;
    Direction d;
    MapTile tile, prev_tile;
    Object *obj;    
    const Monster *m, *to_m;
    int ontoAvatar, ontoMonster;    
    MapCoords coords = from;
    bool isAvatar = (c->location->coords == coords);

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        coords = from;
        ontoAvatar = 0;
        ontoMonster = 0;

        // Move the coordinates in the current direction and test it
        coords.move(d, map);
        
        /* you can always walk off the edge of the map */
        if (MAP_IS_OOB(map, coords)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        obj = mapObjectAt(map, coords);

        /* see if it's trying to move onto the avatar */
        if ((map->flags & SHOW_AVATAR) && (coords == c->location->coords))
            ontoAvatar = 1;
        
        /* see if it's trying to move onto a person or monster */
        else if (obj && (obj->getType() != OBJECT_UNKNOWN))                 
            ontoMonster = 1;
            
        /* get the destination tile */
        if (ontoAvatar)
            tile = (MapTile)c->saveGame->transport;
        else if (ontoMonster)
            tile = obj->getTile();
        else 
            tile = (*c->location->tileAt)(map, coords, WITH_OBJECTS);        

        prev_tile = (*c->location->tileAt)(map, from, WITHOUT_OBJECTS);

        /* get the monster object, if it exists (the one that's moving) */
        m = monsterForTile(transport);
        /* get the other monster object, if it exists (the one that's being moved onto) */        
        to_m = (obj) ? monsterForTile(obj->getTile()) : NULL;

        /* move on if unable to move onto the avatar or another monster */
        if (m && !isAvatar) { /* some monsters/persons have the same tile as the avatar, so we have to adjust */
            if ((ontoAvatar && !monsterCanMoveOntoAvatar(m)) ||
                (ontoMonster && !monsterCanMoveOntoMonsters(m) && !monsterCanMoveOnto(obj->monster)))
                continue;
        }
        /* this really only happens with the avatar */
        else if (ontoMonster && to_m && monsterCanMoveOnto(to_m)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        /* avatar movement */
        if (isAvatar) {
            /* if the transport is a ship, check sailable */
            if (tileIsShip(transport) && tileIsSailable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);
            /* if it is a balloon, check flyable */
            else if (tileIsBalloon(transport) && tileIsFlyable(tile))
                retval = DIR_ADD_TO_MASK(d, retval);        
            /* avatar or horseback: check walkable */
            else if (transport == AVATAR_TILE || tileIsHorse(transport)) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
        }
        
        /* monster movement */
        else if (m) {
            /* flying monsters */
            if (tileIsFlyable(tile) && monsterFlies(m)) {  
                /* FIXME: flying creatures behave differently on the world map? */
                if (mapIsWorldMap(map))
                    retval = DIR_ADD_TO_MASK(d, retval);
                else if (tileIsWalkable(tile) || tileIsSwimable(tile) || tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* swimming monsters and sailing monsters */
            else if (tileIsSwimable(tile) || tileIsSailable(tile)) {
                if (monsterSwims(m) && tileIsSwimable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (monsterSails(m) && tileIsSailable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* ghosts and other incorporeal monsters */
            else if (monsterIsIncorporeal(m)) {
                /* can move anywhere but onto water, unless of course the monster can swim */
                if (!(tileIsSwimable(tile) || tileIsSailable(tile)))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            /* walking monsters */
            else if (monsterWalks(m)) {
                if (tileCanWalkOn(tile, d) &&
                    tileCanWalkOff(prev_tile, d) &&
                    tileIsMonsterWalkable(tile))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }            
        } 
    }

    return retval;
}
