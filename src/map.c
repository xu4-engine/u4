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
#include "monster.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "portal.h"
#include "savegame.h"
#include "tile.h"

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Object *mapObjectAt(const Map *map, int x, int y, int z) {
    Object *obj;
    Object *objAt = NULL;

    for(obj = map->objects; obj; obj = obj->next) {
        if (obj->x == x && obj->y == y && obj->z == z) {
            /* get the most visible object */
            if (objAt && (objAt->objType == OBJECT_UNKNOWN) && (obj->objType != OBJECT_UNKNOWN))
                objAt = obj;
            /* give priority to objects that have the focus */
            else if (objAt && (!objAt->hasFocus) && (obj->hasFocus))
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
const Person *mapPersonAt(const Map *map, int x, int y, int z) {
    Object *obj;

    obj = mapObjectAt(map, x, y, z);
    if (obj && obj->objType == OBJECT_PERSON)
        return obj->person;
    else
        return NULL;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *mapPortalAt(const Map *map, int x, int y, int z, int actionFlags) {
    int i;

    for(i = 0; i < map->n_portals; i++) {
        if (map->portals[i].x == x &&
            map->portals[i].y == y &&
            map->portals[i].z == z &&
            (map->portals[i].trigger_action & actionFlags)) {
            return &(map->portals[i]);
        }
    }
    return NULL;
}

/**
 * Sets the raw tile for the given (x,y,z) coords for the given map.
 * These remain changed until/unless the map is reloaded from file.
 */
void mapSetTileData(const Map *map, int x, int y, int z, unsigned char tile) {
    int index;

    index = x + (y * map->width);
    if ((short)z >= 0)
        index += (map->width * map->height * z);
    map->data[index] = tile;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
unsigned char mapGetTileFromData(const Map *map, int x, int y, int z) {
    int index;

    index = x + (y * map->width);
    if ((short)z >= 0)
        index += (map->width * map->height * z);
    return map->data[index];    
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
unsigned char mapTileAt(const Map *map, int x, int y, int z, int withObjects) {
    unsigned char tile;
    const Annotation *a = annotationAt(x, y, z, map->id);
    Object *obj = mapObjectAt(map, x, y, z);
 
    tile = mapGetTileFromData(map, x, y, z);
    if ((a != NULL) && !a->visual)
        tile = a->tile;
    else if (withObjects && obj && tileIsWalkable(obj->tile))
        tile = obj->tile;
    
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
    Object *obj = mapAddObject(map, person->tile0, person->tile1, person->startx, person->starty, person->startz);

    obj->movement_behavior = person->movement_behavior;
    obj->person = person;
    obj->objType = OBJECT_PERSON;

    return obj;
}

/**
 * Adds a monster object to the given map
 */
Object *mapAddMonsterObject(Map *map, const Monster *monster, int x, int y, int z) {
    Object *obj = mapAddObject(map, monster->tile, monster->tile, x, y, z);

    if (monsterWanders(monster))
        obj->movement_behavior = MOVEMENT_WANDER;
    else if (monsterIsStationary(monster))
        obj->movement_behavior = MOVEMENT_FIXED;
    else obj->movement_behavior = MOVEMENT_ATTACK_AVATAR;

    /* hide camouflaged monsters from view during combat */
    if (monsterCamouflages(monster) && (map->type == MAPTYPE_COMBAT))
        obj->isVisible = 0;

    obj->monster = monster;
    obj->objType = OBJECT_MONSTER;

    return obj;
}

/**
 * Adds an object to the given map
 */
Object *mapAddObject(Map *map, unsigned char tile, unsigned char prevtile, int x, int y, int z) {
    Object *obj = (Object *) malloc(sizeof(Object));

    obj->tile = tile;
    obj->prevtile = prevtile;
    obj->x = x;
    obj->y = y;
    obj->z = z;
    obj->prevx = x;
    obj->prevy = y;
    obj->movement_behavior = MOVEMENT_FIXED;
    obj->objType = OBJECT_UNKNOWN;
    obj->hasFocus = 0;
    obj->isVisible = 1;
    obj->canAnimate = 1;
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
            if (obj->objType == OBJECT_PERSON && !obj->person->permanent)
                free((Person *)obj->person);            

            free(obj);
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
        if ((obj->objType == OBJECT_PERSON) && (obj->person == person)) {
            if (prev)
                prev->next = obj->next;
            else
                map->objects = obj->next;
            free(obj);
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
Object *mapMoveObjects(Map *map, int avatarx, int avatary, int z) {        
    Object *obj = map->objects, *attacker = NULL;        

    for (obj = map->objects; obj; obj = obj->next) {                
        
        /* check if the object is an attacking monster and not
           just a normal, docile person in town or an inanimate object */
        if ((obj->objType != OBJECT_UNKNOWN) && 
           ((obj->objType != OBJECT_MONSTER) || monsterWillAttack(obj->monster)) &&
           ((obj->objType != OBJECT_PERSON) || (obj->movement_behavior == MOVEMENT_ATTACK_AVATAR))) {
            
            if (mapMovementDistance(obj->x, obj->y, avatarx, avatary) == 1) {
                attacker = obj;
                continue;
            }
        }

        /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
        monsterSpecialAction(obj);

        /* Now, move the object according to its movement behavior */
        moveObject(map, obj, avatarx, avatary);        

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
        if (obj->canAnimate && rand() % 2) {
            obj->prevtile = obj->tile;   
            tileAdvanceFrame(&obj->tile);
        }

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
        if (obj->objType == OBJECT_PERSON) {
            obj->tile = obj->person->tile0;
            obj->prevtile = obj->person->tile1;
        } else if (obj->objType == OBJECT_MONSTER) {
            /* we don't want to mess with anything but the prevtile on pirate ships */
            if (obj->monster->id != PIRATE_ID)
                obj->tile = obj->monster->tile;
            obj->prevtile = obj->monster->tile;
        }

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
        if (obj->objType == OBJECT_MONSTER)
            n++;

        obj = obj->next;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int mapGetValidMoves(const Map *map, int from_x, int from_y, int z, unsigned char transport) {
    int retval;
    Direction d;
    unsigned char tile, prev_tile;
    Object *obj;
    int x, y;
    const Monster *m, *to_m;
    int ontoAvatar, ontoMonster;
    int isAvatar = (from_x == c->location->x && from_y == c->location->y && z == c->location->z);

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d++) {
        ontoAvatar = 0;
        ontoMonster = 0;
        x = from_x;
        y = from_y;

        mapDirMove(map, d, &x, &y);
        
        /* you can always walk off the edge of the map */
        if (isAvatar && MAP_IS_OOB(map, x, y)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }        

        obj = mapObjectAt(map, x, y, z);

        /* see if it's trying to move onto the avatar */
        if ((map->flags & SHOW_AVATAR) &&
            x == c->location->x && 
            y == c->location->y) {
            ontoAvatar = 1;
        }
        /* see if it's trying to move onto a person or monster */
        else if (obj && (obj->objType != OBJECT_UNKNOWN))                 
            ontoMonster = 1;
            
        /* get the destination tile */
        if (ontoAvatar)
            tile = (unsigned char)c->saveGame->transport;
        else if (ontoMonster)
            tile = obj->tile;
        else 
            tile = (*c->location->tileAt)(map, x, y, z, WITH_OBJECTS);        

        prev_tile = (*c->location->tileAt)(map, from_x, from_y, z, WITHOUT_OBJECTS);

        /* get the monster object, if it exists (the one that's moving) */
        m = monsterForTile(transport);
        /* get the other monster object, if it exists (the one that's being moved onto) */        
        to_m = (obj) ? monsterForTile(obj->tile) : NULL;

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

/**
 * Find the distance from point a to point b,
 * allowing diagonal movements to calculate
 **/
int mapDistance(int x1, int y1, int x2, int y2) {
    int dist, lowx, highx, lowy, highy;

    dist = 0;
    lowx = (x1 < x2) ? x1 : x2;
    lowy = (y1 < y2) ? y1 : y2;
    highx = (x1 > x2) ? x1 : x2;
    highy = (y1 > y2) ? y1 : y2;

    while ((lowx < highx) || (lowy < highy))
    {
        if (lowx < highx) lowx++;
        if (lowy < highy) lowy++;
        dist++;
    }

    return dist;
}

/**
 * Find the number of moves it would take to get
 * from point a to point b (no diagonals allowed)
 */
int mapMovementDistance(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2),
        dy = abs(y1 - y2);

    return (dx + dy);
}

/**
 * Moves x and y in 'dir' direction on the map, wrapping if necessary
 * Returns 1 if succeeded, 0 if map doesn't wrap and x or y was moved
 * beyond the borders of the map
 */
int mapDirMove(const Map *map, Direction dir, int *x, int *y) {
    int newx = *x,
        newy = *y,
        wraps = map->border_behavior == BORDER_WRAP;

    dirMove(dir, &newx, &newy);
    if (MAP_IS_OOB(map, newx, newy) && wraps)        
        mapWrapCoordinates(map, &newx, &newy);    

    *x = newx;
    *y = newy;

    return 1;
}

/**
 * Wraps x,y coordinates on a map if necessary and possible
 * Returns 1 if succeeded, 0 if not needed or not possible
 */
int mapWrapCoordinates(const Map *map, int *x, int *y) {
    if (map->border_behavior == BORDER_WRAP) {
        if (*x < 0) *x += map->width;
        if (*x >= (int)map->width) *x -= map->width;
        if (*y < 0) *y += map->height;
        if (*y >= (int)map->height) *y -= map->height;
        return 1;
    }
    return 0;
}
