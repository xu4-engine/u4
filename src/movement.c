/**
 * $Id$
 */

#include <stdlib.h>

#include "movement.h"

#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "dungeon.h"
#include "location.h"
#include "mapmgr.h"
#include "monster.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "ttype.h"

int collisionOverride = 0;

/**
 * Attempt to move the avatar in the given direction.  User event
 * should be set if the avatar is being moved in response to a
 * keystroke.  Returns zero if the avatar is blocked.
 */
MoveReturnValue moveAvatar(Direction dir, int userEvent) {    
    int newx, newy;
    int slowed = 0;
    unsigned char temp;
    SlowedType slowedType = SLOWED_BY_TILE;
    Object *destObj;    

    /* Check to see if we're on the balloon */
    if (c->transportContext == TRANSPORT_BALLOON && userEvent)
        return MOVE_DRIFT_ONLY | MOVE_END_TURN;
        
    if (c->transportContext == TRANSPORT_SHIP)
        slowedType = SLOWED_BY_WIND;
    else if (c->transportContext == TRANSPORT_BALLOON)
        slowedType = SLOWED_BY_NOTHING;    

    /* if you're on ship, you must turn first! */
    if (c->transportContext == TRANSPORT_SHIP) {
        if (tileGetDirection((unsigned char)c->saveGame->transport) != dir) {
	        temp = (unsigned char)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	        c->saveGame->transport = temp;            
            return MOVE_TURNED | MOVE_END_TURN;
        }
    }
    
    /* chance direction of horse, if necessary */
    if (c->transportContext == TRANSPORT_HORSE) {
        if ((dir == DIR_WEST || dir == DIR_EAST) &&
            tileGetDirection((unsigned char)c->saveGame->transport) != dir) {
	    temp = (unsigned char)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	    c->saveGame->transport = temp;
        }
    }

    /* figure out our new location we're trying to move to */
    newx = c->location->x;
    newy = c->location->y;

    dirMove(dir, &newx, &newy);    

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        switch (c->location->map->border_behavior) {        
        case BORDER_WRAP:
            mapWrapCoordinates(c->location->map, &newx, &newy);
            break;

        case BORDER_EXIT2PARENT:            
            return MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED;

        case BORDER_FIXED:
            if (newx < 0 || newx >= (int) c->location->map->width)
                newx = c->location->x;
            if (newy < 0 || newy >= (int) c->location->map->height)
                newy = c->location->y;
            break;
        }
    }

    if (!collisionOverride && !c->saveGame->balloonstate) {
        int movementMask;

        movementMask = mapGetValidMoves(c->location->map, c->location->x, c->location->y, c->location->z, (unsigned char)c->saveGame->transport);
        /* See if movement was blocked */
        if (!DIR_IN_MASK(dir, movementMask))
            return MOVE_BLOCKED | MOVE_END_TURN;

        /* Are we slowed by terrain or by wind direction? */
        switch(slowedType) {
        case SLOWED_BY_TILE:
            slowed = slowedByTile((*c->location->tileAt)(c->location->map, newx, newy, c->location->z, WITHOUT_OBJECTS));
            break;
        case SLOWED_BY_WIND:
            slowed = slowedByWind(dir);
            break;
        case SLOWED_BY_NOTHING:
        default:
            break;
        }
        
        if (slowed)            
            return MOVE_SLOWED | MOVE_END_TURN;
    }

    /* move succeeded */
    c->location->x = newx;
    c->location->y = newy;

    /* if the avatar moved onto a monster (whirlpool, twister), then do the monster's special effect */
    destObj = mapObjectAt(c->location->map, newx, newy, c->location->z);
    if (destObj && destObj->objType == OBJECT_MONSTER)
        monsterSpecialEffect(destObj);    

    return MOVE_SUCCEEDED | MOVE_END_TURN;
}

/**
 * Moves the avatar while in dungeon view
 */
MoveReturnValue moveAvatarInDungeon(Direction dir, int userEvent) {    
    int newx, newy;    
    Direction realDir = dirNormalize(c->saveGame->orientation, dir); /* get our real direction */    
    
    /* we're not in a dungeon, failed! */
    if (c->location->context != CTX_DUNGEON)
        return 0;
        
    /* you must turn first! */
    if (c->saveGame->orientation != realDir &&
        c->saveGame->orientation != dirReverse(realDir)) {        
        
        c->saveGame->orientation = realDir;
        return MOVE_TURNED;
    }
    
    /* figure out our new location */
    newx = c->location->x;
    newy = c->location->y;

    dirMove(realDir, &newx, &newy);    

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        switch (c->location->map->border_behavior) {        
        case BORDER_WRAP:
            mapWrapCoordinates(c->location->map, &newx, &newy);
            break;

        case BORDER_EXIT2PARENT:
            return MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED;

        case BORDER_FIXED:
            if (newx < 0 || newx >= (int) c->location->map->width)
                newx = c->location->x;
            if (newy < 0 || newy >= (int) c->location->map->height)
                newy = c->location->y;
            break;
        }
    }

    if (!collisionOverride) {
        int movementMask;

        movementMask = mapGetValidMoves(c->location->map, c->location->x, c->location->y, c->location->z, (unsigned char)c->saveGame->transport);
        if (!DIR_IN_MASK(realDir, movementMask))            
            return MOVE_BLOCKED | MOVE_END_TURN;        
    }

    c->location->x = newx;
    c->location->y = newy;    

    return MOVE_SUCCEEDED | MOVE_END_TURN;
}

/**
 * Moves an object on the map according to its movement behavior
 * Returns 1 if the object was moved successfully, 0 if slowed,
 * tile direction changed, or object simply cannot move
 * (fixed objects, nowhere to go, etc.)
 */
int moveObject(Map *map, Object *obj, int avatarx, int avatary) {
    int dir,        
        dirmask = DIR_NONE,
        oldtile = obj->tile;
    unsigned int newx = obj->x,
                 newy = obj->y,
                 z = obj->z;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    obj->prevx = obj->x;
    obj->prevy = obj->y;
    
    /* determine a direction depending on the object's movement behavior */
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
            if (dir = tileGetDirection(obj->tile))
                break;

        dir = dirFindPathToTarget(newx, newy, avatarx, avatary, dirmask);
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
        slowed = slowedByTile((*c->location->tileAt)(map, newx, newy, obj->z, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(tileGetDirection(obj->tile));
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }
    
    /* see if the object needed to turn instead of move */
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
    unsigned int newx = obj->x,
                 newy = obj->y,
                 valid_dirs = mapGetValidMoves(map, newx, newy, c->location->z, obj->tile),
                 dir = DIR_NONE;
    CombatAction action = (CombatAction)act;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    /* fixed objects cannot move */
    if (obj->movement_behavior == MOVEMENT_FIXED)
        return 0;

    if (action == CA_FLEE)
        /* run away from our target instead! */
        dir = dirFindPathAwayFromTarget(newx, newy, targetx, targety, valid_dirs);
    
    else if (action == CA_ADVANCE)
    {
        // If they're not fleeing, make sure they don't flee on accident
        if (newx == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        else if (newx >= map->width - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (newy == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        else if (newy >= map->height - 1)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);        

        dir = dirFindPathToTarget(newx, newy, targetx, targety, valid_dirs);
    }

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
        slowed = slowedByTile((*c->location->tileAt)(map, newx, newy, c->location->z, WITHOUT_OBJECTS));
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
 * Moves a party member during combat screens
 */
int movePartyMember(Direction dir, int userEvent) {
    MoveReturnValue result = MOVE_SUCCEEDED;
    int newx, newy;
    int movementMask;
    int member = combatInfo.partyFocus;    

    /* find our new location */
    newx = combatInfo.party[member].obj->x;
    newy = combatInfo.party[member].obj->y;
    dirMove(dir, &newx, &newy);    

    if (MAP_IS_OOB(c->location->map, newx, newy)) {
        int sameExit = (!combatInfo.dungeonRoom || (combatInfo.exitDir == DIR_NONE) || (dir == combatInfo.exitDir));
        if (sameExit) {
            
            /* if in a win-or-lose battle and not camping, then it can be bad to flee while healthy */
            if (combatInfo.winOrLose && !combatInfo.camping) {
                /* A fully-healed party member fled from an evil monster :( */
                if (combatInfo.monster && monsterIsEvil(combatInfo.monster) && 
                    c->saveGame->players[member].hp == c->saveGame->players[member].hpMax)
                    playerAdjustKarma(c->saveGame, KA_HEALTHY_FLED_EVIL);
            }

            combatInfo.exitDir = dir;
            mapRemoveObject(c->location->map, combatInfo.party[member].obj);
            combatInfo.party[member].obj = NULL;
            return MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE | MOVE_SUCCEEDED | MOVE_END_TURN;
        }
        else return MOVE_MUST_USE_SAME_EXIT | MOVE_END_TURN;
    }

    movementMask = mapGetValidMoves(c->location->map, combatInfo.party[member].obj->x, combatInfo.party[member].obj->y, c->location->z, combatInfo.party[member].obj->tile);
    if (!DIR_IN_MASK(dir, movementMask))
        return MOVE_BLOCKED | MOVE_END_TURN;

    /* is the party member slowed? */
    if (!slowedByTile((*c->location->tileAt)(c->location->map, newx, newy, c->location->z, WITHOUT_OBJECTS)))
    {
        combatInfo.party[member].obj->x = newx;
        combatInfo.party[member].obj->y = newy;

        /* handle dungeon room triggers */
        if (combatInfo.dungeonRoom) {
            int i;
            Trigger *triggers = c->location->prev->map->dungeon->currentRoom->triggers;            

            for (i = 0; i < 4; i++) {
                const Monster *m = monsterForTile(triggers[i].tile);

                /* FIXME: when a monster is created by a trigger, it can be created over and over and over...
                   how do we fix this? */

                /* see if we're on a trigger */
                if (triggers[i].x == newx && triggers[i].y == newy) {
                    /* change the tiles! */
                    if (triggers[i].change_x1 || triggers[i].change_y1) {                    
                        /*if (m) combatAddMonster(m, triggers[i].change_x1, triggers[i].change_y1, c->location->z);
                        else*/ annotationAdd(triggers[i].change_x1, triggers[i].change_y1, c->location->z, 0, triggers[i].tile);
                    }
                    if (triggers[i].change_x2 || triggers[i].change_y2) {
                        /*if (m) combatAddMonster(m, triggers[i].change_x2, triggers[i].change_y2, c->location->z);
                        else*/ annotationAdd(triggers[i].change_x2, triggers[i].change_y2, c->location->z, 0, triggers[i].tile);
                    }
                }
            }
        }    
    }
    else return MOVE_SLOWED | MOVE_END_TURN;
        
    return result;
}
 
/**
 * Default handler for slowing movement.
 * Returns 1 if slowed, 0 if not slowed
 */
int slowedByTile(unsigned char tile) {
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
