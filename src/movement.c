/**
 * $Id$
 */

#include <stdlib.h>

#include "movement.h"

#include "combat.h"
#include "context.h"
#include "location.h"
#include "monster.h"
#include "object.h"
#include "savegame.h"
#include "ttype.h"

/**
 * Moves an object on the map according to its movement behavior
 * Returns 1 if the object was moved successfully, 0 if slowed,
 * tile direction changed, or object simply cannot move
 * (fixed objects, nowhere to go, etc.)
 */
int moveObject(Map *map, Object *obj, int avatarx, int avatary) {
    int dir,
        newx = obj->x,
        newy = obj->y,
        z = obj->z,
        dirmask = DIR_NONE,
        oldtile = obj->tile;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    obj->prevx = obj->x;
    obj->prevy = obj->y;
    
    /* determine a direction depending on its movement behavior */
    dir = DIR_NONE;
    switch (obj->movement_behavior) {
    case MOVEMENT_FIXED:
        break;

    case MOVEMENT_WANDER:  
        /* World map wandering creatures always move, whereas
           town creatures that wander sometimes stay put */
        if (mapIsWorldMap(map) || rand() % 2 == 0)
            dir = dirRandomDir(mapGetValidMoves(map, newx, newy, z, obj->tile));                
        break;

    case MOVEMENT_FOLLOW_AVATAR:
    case MOVEMENT_ATTACK_AVATAR:
        dirmask = mapGetValidMoves(map, newx, newy, z, obj->tile);
        
        /* If the pirate ship turned last move instead of moving, this time it must
           try to move, not turn again */
        if (tileIsPirateShip(obj->tile) && DIR_IN_MASK(tileGetDirection(obj->tile), dirmask) &&
            (obj->tile != obj->prevtile) && (obj->prevx == obj->x) && (obj->prevy == obj->y))
            if ((dir = dirFindPath(newx, newy, avatarx, avatary, 1 << tileGetDirection(obj->tile))))
                break;

        dir = dirFindPath(newx, newy, avatarx, avatary, dirmask);
        break;
    }
    
    /* now, get a new x and y for the object */
    if (dir)
        dirMove(dir, &newx, &newy);
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */    
    
    if (obj->objType == OBJECT_MONSTER)
        slowedType = obj->monster->slowedType;
    
    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(mapTileAt(map, newx, newy, obj->z));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(tileGetDirection(obj->tile));
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }
    
    /* see if the object needed to turn */
    if (tileSetDirection(&obj->tile, dir)) {
        obj->prevtile = oldtile;
        return 0;
    }
    
    /* was the object slowed? */
    if (slowed)
        return 0;

    if ((newx != obj->x || newy != obj->y) &&
        newx >= 0 && newx < map->width &&
        newy >= 0 && newy < map->height) {

        if (newx != obj->x ||
            newy != obj->y) {
            obj->prevx = obj->x;
            obj->prevy = obj->y;
        }
        obj->x = newx;
        obj->y = newy;
    }

    return 1;
}

/**
 * Moves an object in combat according to its chosen combat action
 */
int moveCombatObject(int act, Map *map, Object *obj, int targetx, int targety) {
    int newx = obj->x,
        newy = obj->y,
        valid_dirs = mapGetValidMoves(c->location->map, newx, newy, c->location->z, obj->tile),
        dir = DIR_NONE;
    CombatAction action = (CombatAction)act;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    /* fixed objects cannot move */
    if (obj->movement_behavior == MOVEMENT_FIXED)
        return 0;

    if (action == CA_FLEE)
        dir = dirFindPathToEdge(newx, newy, c->location->map->width, c->location->map->height, valid_dirs);
    else if (action == CA_ADVANCE)
    {
        // If they're not fleeing, make sure they don't flee on accident
        if (newx == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        else if (newx >= c->location->map->width - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (newy == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        else if (newy >= c->location->map->height - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);        

        dir = dirFindPath(newx, newy, targetx, targety, valid_dirs);
    }

    if (dir)
        dirMove(dir, &newx, &newy);
    else
        return 0;    

    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(mapTileAt(c->location->map, newx, newy, c->location->z));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(tileGetDirection(obj->tile));
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }

    /* if the object wan't slowed... */
    if (!slowed) {
        if (newx != obj->x ||
            newy != obj->y) {
            obj->prevx = obj->x;
            obj->prevy = obj->y;
        }
        obj->x = newx;
        obj->y = newy;

        return 1;
    }

    return 0;
}
 
/**
 * Default handler for slowing movement.
 * Returns 1 if slowed, 0 if not slowed
 */
int slowedByTile(int tile) {
    int slow;
    
    switch (tileGetSpeed(tile)) {    
    case SLOW:
        slow = (rand() % 8) == 0;
        break;
    case VSLOW:
        slow = (rand() % 4) == 0;
        break;
    case VVSLOW:
        slow = (rand() % 2) == 0;
        break;
    case FAST:
    default:
        slow = 0;
        break;
    }

    return slow;
}

/**
 * Slowed depending on the direction of object with respect to wind direction
 * Returns 1 if slowed, 0 if not slowed
 */
int slowedByWind(int direction) {
    /* 1 of 4 moves while trying to move into the wind succeeds */
    if (direction == c->windDirection)
        return (c->saveGame->moves % 4) != 0;
    /* 1 of 4 moves while moving directly away from wind fails */
    else if (direction == dirReverse((Direction) c->windDirection))
        return (c->saveGame->moves % 4) == 3;    
    else
        return 0;
}
