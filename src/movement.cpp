/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "movement.h"

#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "location.h"
#include "mapmgr.h"
#include "creature.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "tile.h"
#include "utils.h"

bool collisionOverride = false;

/**
 * Attempt to move the avatar in the given direction.  User event
 * should be set if the avatar is being moved in response to a
 * keystroke.  Returns zero if the avatar is blocked.
 */
MoveReturnValue moveAvatar(Direction dir, int userEvent) {    
    MapCoords newCoords;
    int slowed = 0;
    MapTile temp;
    SlowedType slowedType = SLOWED_BY_TILE;
    Object *destObj;

    /* Check to see if we're on the balloon */
    if (c->transportContext == TRANSPORT_BALLOON && userEvent)
        return (MoveReturnValue)(MOVE_DRIFT_ONLY | MOVE_END_TURN);
        
    if (c->transportContext == TRANSPORT_SHIP)
        slowedType = SLOWED_BY_WIND;
    else if (c->transportContext == TRANSPORT_BALLOON)
        slowedType = SLOWED_BY_NOTHING;

    /* if you're on ship, you must turn first! */
    if (c->transportContext == TRANSPORT_SHIP) {
        if (tileGetDirection((MapTile)c->saveGame->transport) != dir) {
	        temp = (MapTile)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	        c->saveGame->transport = temp;
            return (MoveReturnValue)(MOVE_TURNED | MOVE_END_TURN);
        }
    }
    
    /* change direction of horse, if necessary */
    if (c->transportContext == TRANSPORT_HORSE) {
        if ((dir == DIR_WEST || dir == DIR_EAST) &&
            tileGetDirection((MapTile)c->saveGame->transport) != dir) {
	    temp = (MapTile)c->saveGame->transport;
            tileSetDirection(&temp, dir);
	    c->saveGame->transport = temp;
        }
    }

    /* figure out our new location we're trying to move to */
    newCoords = c->location->coords;    
    newCoords.move(dir, c->location->map);    

    /* see if we moved off the map */
    if (MAP_IS_OOB(c->location->map, newCoords))
        return (MoveReturnValue)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);

    if (!collisionOverride && !c->saveGame->balloonstate) {
        int movementMask;

        movementMask = c->location->map->getValidMoves(c->location->coords, (MapTile)c->saveGame->transport);
        /* See if movement was blocked */
        if (!DIR_IN_MASK(dir, movementMask))
            return (MoveReturnValue)(MOVE_BLOCKED | MOVE_END_TURN);

        /* Are we slowed by terrain or by wind direction? */
        switch(slowedType) {
        case SLOWED_BY_TILE:
            slowed = slowedByTile(c->location->map->tileAt(newCoords, WITHOUT_OBJECTS));
            break;
        case SLOWED_BY_WIND:
            slowed = slowedByWind(dir);
            break;
        case SLOWED_BY_NOTHING:
        default:
            break;
        }
        
        if (slowed)            
            return (MoveReturnValue)(MOVE_SLOWED | MOVE_END_TURN);
    }

    /* move succeeded */
    c->location->coords = newCoords;    

    /* if the avatar moved onto a creature (whirlpool, twister), then do the creature's special effect */
    destObj = c->location->map->objectAt(newCoords);
    if (destObj && destObj->getType() == OBJECT_CREATURE) {
        Creature *m = dynamic_cast<Creature*>(destObj);
        m->specialEffect();
    }

    return (MoveReturnValue)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves the avatar while in dungeon view
 */
MoveReturnValue moveAvatarInDungeon(Direction dir, int userEvent) {    
    MapCoords newCoords;
    Direction realDir = dirNormalize((Direction)c->saveGame->orientation, dir); /* get our real direction */  
    int advancing = realDir == c->saveGame->orientation,
        retreating = realDir == dirReverse((Direction)c->saveGame->orientation);
    MapTile tile;
    
    /* we're not in a dungeon, failed! */
    ASSERT(c->location->context & CTX_DUNGEON, "moveAvatarInDungeon() called outside of dungeon, failed!");    
        
    /* you must turn first! */
    if (!advancing && !retreating) {        
        c->saveGame->orientation = realDir;
        return MOVE_TURNED;
    }
    
    /* figure out our new location */
    newCoords = c->location->coords;    
    newCoords.move(realDir, c->location->map);

    tile = c->location->map->tileAt(newCoords, WITH_OBJECTS);

    /* see if we moved off the map (really, this should never happen in a dungeon) */
    if (MAP_IS_OOB(c->location->map, newCoords))
        return (MoveReturnValue)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);

    if (!collisionOverride) {
        int movementMask;        
        
        movementMask = c->location->map->getValidMoves(c->location->coords, (MapTile)c->saveGame->transport);        

        if (advancing && !tileCanWalkOn(tile, DIR_ADVANCE))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);
        else if (retreating && !tileCanWalkOn(tile, DIR_RETREAT))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);

        if (!DIR_IN_MASK(realDir, movementMask))
            return (MoveReturnValue)(MOVE_BLOCKED | MOVE_END_TURN);
    }

    /* move succeeded */
    c->location->coords = newCoords;    

    return (MoveReturnValue)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves an object on the map according to its movement behavior
 * Returns 1 if the object was moved successfully, 0 if slowed,
 * tile direction changed, or object simply cannot move
 * (fixed objects, nowhere to go, etc.)
 */
int moveObject(Map *map, Creature *obj, MapCoords avatar) {
    int dirmask = DIR_NONE;
    Direction dir;
    MapCoords new_coords = obj->getCoords();    
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;    
    
    /* determine a direction depending on the object's movement behavior */
    dir = DIR_NONE;
    switch (obj->getMovementBehavior()) {
    case MOVEMENT_FIXED:
        break;

    case MOVEMENT_WANDER:
        /* World map wandering creatures always move, whereas
           town creatures that wander sometimes stay put */
        if (map->isWorldMap() || xu4_random(2) == 0)
            dir = dirRandomDir(map->getValidMoves(new_coords, obj->getTile()));
        break;

    case MOVEMENT_FOLLOW_AVATAR:
    case MOVEMENT_ATTACK_AVATAR:
        dirmask = map->getValidMoves(new_coords, obj->getTile());
        
        /* If the pirate ship turned last move instead of moving, this time it must
           try to move, not turn again */
        if (tileIsPirateShip(obj->getTile()) && DIR_IN_MASK(tileGetDirection(obj->getTile()), dirmask) &&
            (obj->getTile() != obj->getPrevTile()) && (obj->getPrevCoords() == obj->getCoords())) {
            dir = tileGetDirection(obj->getTile());
            break;
        }

        dir = new_coords.pathTo(avatar, dirmask, true, c->location->map);
        break;
    }
    
    /* now, get a new x and y for the object */
    if (dir)
        new_coords.move(dir, c->location->map);        
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */   
    if (obj->getType() == OBJECT_CREATURE)
        slowedType = obj->slowedType;
    
    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(tileGetDirection(obj->getTile()));
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }
    
    obj->setPrevCoords(obj->getCoords());
    
    /* see if the object needed to turn instead of move */
    if (obj->setDirection(dir))
        return 0;    
    
    /* was the object slowed? */
    if (slowed)
        return 0;

    /**
     * Set the new coordinates
     */ 
    if (!(new_coords == obj->getCoords()) && 
        !MAP_IS_OOB(map, new_coords))
        obj->setCoords(new_coords);    

    return 1;
}

/**
 * Moves an object in combat according to its chosen combat action
 */
int moveCombatObject(int act, Map *map, Creature *obj, MapCoords target) {
    MapCoords new_coords = obj->getCoords();
    int valid_dirs = map->getValidMoves(new_coords, obj->getTile());
    Direction dir;
    CombatAction action = (CombatAction)act;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    /* fixed objects cannot move */
    if (obj->getMovementBehavior() == MOVEMENT_FIXED)
        return 0;

    if (action == CA_FLEE)
        /* run away from our target instead! */
        dir = new_coords.pathAway(target, valid_dirs);
    
    else if (action == CA_ADVANCE)
    {
        // If they're not fleeing, make sure they don't flee on accident
        if (new_coords.x == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        else if (new_coords.x >= (signed)(map->width - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (new_coords.y == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        else if (new_coords.y >= (signed)(map->height - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);        

        dir = new_coords.pathTo(target, valid_dirs);
    }

    if (dir)
        new_coords.move(dir, c->location->map);
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */   
    if (obj->getType() == OBJECT_CREATURE)
        slowedType = obj->slowedType;

    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(tileGetDirection(obj->getTile()));
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }

    /* if the object wan't slowed... */
    if (!slowed) {        
        // Set the new coordinates
        obj->setCoords(new_coords);

        return 1;
    }

    return 0;
}

/**
 * Moves a party member during combat screens
 */
MoveReturnValue movePartyMember(Direction dir, int userEvent) {
    CombatController *ct = c->combat;
    CombatMap *cm = getCombatMap();
    MoveReturnValue result = MOVE_SUCCEEDED;    
    int movementMask;
    int member = ct->getFocus();
    MapCoords newCoords;
    PartyMemberVector *party = ct->getParty();

    /* find our new location */
    newCoords = (*party)[member]->getCoords();
    newCoords.move(dir, c->location->map);

    if (MAP_IS_OOB(c->location->map, newCoords)) {
        bool sameExit = (!cm->isDungeonRoom() || (ct->getExitDir() == DIR_NONE) || (dir == ct->getExitDir()));
        if (sameExit) {
            /* if in a win-or-lose battle and not camping, then it can be bad to flee while healthy */
            if (ct->isWinOrLose() && !ct->isCamping()) {
                /* A fully-healed party member fled from an evil creature :( */
                if (ct->getCreature() && ct->getCreature()->isEvil() && 
                    c->party->member(member)->getHp() == c->party->member(member)->getMaxHp())
                    c->party->adjustKarma(KA_HEALTHY_FLED_EVIL);
            }

            ct->setExitDir(dir);
            c->location->map->removeObject((*party)[member]);
            (*party)[member] = NULL;
            return (MoveReturnValue)(MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE | MOVE_SUCCEEDED | MOVE_END_TURN);
        }
        else return (MoveReturnValue)(MOVE_MUST_USE_SAME_EXIT | MOVE_END_TURN);
    }

    movementMask = c->location->map->getValidMoves((*party)[member]->getCoords(), (*party)[member]->getTile());
    if (!DIR_IN_MASK(dir, movementMask))
        return (MoveReturnValue)(MOVE_BLOCKED | MOVE_END_TURN);

    /* is the party member slowed? */
    if (!slowedByTile(c->location->map->tileAt(newCoords, WITHOUT_OBJECTS)))
    {
        /* move succeeded */        
        (*party)[member]->setCoords(newCoords);

        /* handle dungeon room triggers */
        if (cm->isDungeonRoom()) {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(c->location->prev->map);
            int i;
            Trigger *triggers = dungeon->currentRoom->triggers;            

            for (i = 0; i < 4; i++) {
                /*const Creature *m = creatures.getByTile(triggers[i].tile);*/

                /* FIXME: when a creature is created by a trigger, it can be created over and over and over...
                   how do we fix this? */
                MapCoords trigger(triggers[i].x, triggers[i].y, c->location->coords.z);

                /* see if we're on a trigger */
                if (newCoords == trigger) {
                    MapCoords change1(triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z),
                              change2(triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z);

                    /**
                     * Remove any previous annotations placed at our target coordinates
                     */ 
                    c->location->map->annotations->remove(c->location->map->annotations->allAt(change1));
                    c->location->map->annotations->remove(c->location->map->annotations->allAt(change2));

                    /* change the tiles! */
                    if (change1.x || change1.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z);
                        else*/ c->location->map->annotations->add(change1, triggers[i].tile);
                    }
                    if (change2.x || change2.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z);
                        else*/ c->location->map->annotations->add(change2, triggers[i].tile);
                    }
                }
            }
        }    
    }
    else return (MoveReturnValue)(MOVE_SLOWED | MOVE_END_TURN);
        
    return result;
}
 
/**
 * Default handler for slowing movement.
 * Returns 1 if slowed, 0 if not slowed
 */
int slowedByTile(MapTile tile) {
    int slow;
    
    switch (tileGetSpeed(tile)) {    
    case SLOW:
        slow = xu4_random(8) == 0;
        break;
    case VSLOW:
        slow = xu4_random(4) == 0;
        break;
    case VVSLOW:
        slow = xu4_random(2) == 0;
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
