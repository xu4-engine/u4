/**
 * $Id$
 */

#include <stdlib.h>

#include "movement.h"
#include "ttype.h"
#include "context.h"
#include "savegame.h"
#include "location.h"
#include "object.h"
#include "combat.h"
#include "monster.h"

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
    SlowedCallback slowedCallback;
    int slowedParam;

    obj->prevx = obj->x;
    obj->prevy = obj->y;
    
    /* determine a direction depending on its movement behavior */
    dir = DIR_NONE;
    switch (obj->movement_behavior) {
    case MOVEMENT_FIXED:
        break;

    case MOVEMENT_WANDER:        
        dir = dirRandomDir(mapGetValidMoves(map, newx, newy, z, obj->tile));                
        break;

    case MOVEMENT_FOLLOW_AVATAR:
    case MOVEMENT_ATTACK_AVATAR:
        dirmask = mapGetValidMoves(map, newx, newy, z, obj->tile);
        
        /* If the pirate ship turned last move instead of moving, this time it must
           try to move not turn again */
        if (tileIsPirateShip(obj->tile) && DIR_IN_MASK(tileGetDirection(obj->tile), dirmask) &&
            (obj->tile != obj->prevtile) && (obj->prevx == obj->x) && (obj->prevy == obj->y))
            if (dir = dirFindPath(newx, newy, avatarx, avatary, 1 << tileGetDirection(obj->tile)))
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
    slowedCallback = &slowedHandlerDefault;
    slowedParam = mapTileAt(map, newx, newy, obj->z);

    if (obj->objType == OBJECT_MONSTER) {
        if (monsterFlies(obj->monster))
            slowedCallback = &slowedHandlerNone;
        else if (tileIsPirateShip(obj->monster->tile)) {
            slowedCallback = &slowedHandlerWind;
            slowedParam = tileGetDirection(obj->tile);
        }
    }
    
    /* see if the object needed to turn */
    if (tileSetDirection(&obj->tile, dir)) {
        obj->prevtile = oldtile;
        return 0;
    }
    
    /* was the object slowed? */
    if ((*slowedCallback)(slowedParam))
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
    
    SlowedCallback slowedCallback;
    int slowedParam;

    if (action == CA_FLEE)
        dir = dirFindPathToEdge(newx, newy, c->location->map->width, c->location->map->height, valid_dirs);
    else if (action == CA_ADVANCE)
    {
        // If they're not fleeing, make sure they don't flee on accident
        if (newx == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        if (newx == c->location->map->width - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (newy == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        if (newy == c->location->map->height - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);

        dir = dirFindPath(newx, newy, targetx, targety, valid_dirs);
    }

    if (dir)
        dirMove(dir, &newx, &newy);
    else
        return 0;

    slowedCallback = &slowedHandlerDefault;
    slowedParam = mapTileAt(c->location->map, newx, newy, c->location->z);
    if (obj->objType == OBJECT_MONSTER && monsterFlies(obj->monster))
        slowedCallback = &slowedHandlerNone;

    if (!(*slowedCallback)(slowedParam)) {
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

int slowedHandlerDefault(int tile) {
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
 * For objects that cannot be slowed (flying monsters, etc.)
 */

int slowedHandlerNone(int unused) {
    return 0;
}

/**
 * Slowed depending on the direction of object with respect to wind direction
 * Returns 1 if slowed, 0 if not slowed
 */

int slowedHandlerWind(int direction) {
    /* 1 of 4 moves while trying to move into the wind succeeds */
    if (direction == c->windDirection)
        return (c->saveGame->moves % 4) != 0;
    /* 1 of 4 moves while moving directly away from wind fails */
    else if (direction == dirReverse((Direction) c->windDirection))
        return (c->saveGame->moves % 4) == 3;    
    else
        return 0;
}
