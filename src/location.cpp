/*
 * $Id$
 */

#include <list>
#include <map>
#include <set>

#include "location.h"

#include "context.h"
#include "combat.h"
#include "debug.h"
#include "dungeon.h"
#include "settings.h"
#include "tileset.h"
#include "xu4.h"


bool collisionOverride = false;

/**
 * Default handler for slowing movement.
 * Returns true if slowed, false if not slowed
 */
static bool slowedByTile(const Tile *tile) {
    bool slow;

    switch (tile->getSpeed()) {
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
        slow = false;
        break;
    }

    return slow;
}

/**
 * Slowed depending on the direction of object with respect to wind direction
 * Returns true if slowed, false if not slowed
 */
static bool slowedByWind(int direction) {
    /* 1 of 4 moves while trying to move into the wind succeeds */
    if (direction == c->windDirection)
        return (c->saveGame->moves % 4) != 0;
    /* 1 of 4 moves while moving directly away from wind fails */
    else if (direction == dirReverse((Direction) c->windDirection))
        return (c->saveGame->moves % 4) == 3;
    else
        return false;
}

/**
 * Attempt to move the avatar in the given direction.  User event
 * should be set if the avatar is being moved in response to a
 * keystroke.  Returns zero if the avatar is blocked.
 */
static void moveAvatar(MoveEvent &event) {
    Coords newCoords;
    int slowed = 0;
    SlowedType slowedType = SLOWED_BY_TILE;

    /* Check to see if we're on the balloon */
    if (c->transportContext == TRANSPORT_BALLOON && event.userEvent) {
        event.result = (MoveResult)(MOVE_DRIFT_ONLY | MOVE_END_TURN);
        return;
    }

    if (c->transportContext == TRANSPORT_SHIP)
        slowedType = SLOWED_BY_WIND;
    else if (c->transportContext == TRANSPORT_BALLOON)
        slowedType = SLOWED_BY_NOTHING;

    /* if you're on ship, you must turn first! */
    if (c->transportContext == TRANSPORT_SHIP) {
        if (c->party->getDirection() != event.dir) {
            c->party->setDirection(event.dir);
            event.result = (MoveResult)(MOVE_TURNED | MOVE_END_TURN);
            return;
        }
    }

    /* change direction of horse, if necessary */
    if (c->transportContext == TRANSPORT_HORSE) {
        if ((event.dir == DIR_WEST || event.dir == DIR_EAST) && (c->party->getDirection() != event.dir))
            c->party->setDirection(event.dir);
    }

    /* figure out our new location we're trying to move to */
    newCoords = c->location->coords;
    map_move(newCoords, event.dir, c->location->map);

    /* see if we moved off the map */
    if (MAP_IS_OOB(c->location->map, newCoords)) {
        event.result = (MoveResult)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);
        return;
    }

    if (!collisionOverride && !c->party->isFlying()) {
        int movementMask = c->location->map->getValidMoves(c->location->coords, c->party->getTransport());
        /* See if movement was blocked */
        if (!DIR_IN_MASK(event.dir, movementMask)) {
            event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
            return;
        }

        /* Are we slowed by terrain or by wind direction? */
        switch(slowedType) {
        case SLOWED_BY_TILE:
          // TODO: CHEST: Make a user option to not make chests always fast to
          // travel over
            slowed = slowedByTile(c->location->map->tileTypeAt(newCoords, WITH_OBJECTS));
            break;
        case SLOWED_BY_WIND:
            slowed = slowedByWind(event.dir);
            break;
        case SLOWED_BY_NOTHING:
        default:
            break;
        }

        if (slowed) {
            event.result = (MoveResult)(MOVE_SLOWED | MOVE_END_TURN);
            return;
        }
    }

    /* move succeeded */
    c->location->coords = newCoords;

    /* if the avatar moved onto a creature (whirlpool, twister), then do the creature's special effect (this current code does double damage according to changeset 2753.

    Object *destObj = c->location->map->objectAt(newCoords);
    if (destObj && destObj->getType() == Object::CREATURE) {
        Creature *m = dynamic_cast<Creature*>(destObj);
        //m->specialEffect();
    }
    */

    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves the avatar while in dungeon view
 */
static void moveAvatarInDungeon(MoveEvent &event) {
    Coords newCoords;
    Direction realDir = dirNormalize((Direction)c->saveGame->orientation, event.dir); /* get our real direction */
    int advancing = realDir == c->saveGame->orientation,
        retreating = realDir == dirReverse((Direction)c->saveGame->orientation);
    const Tile* tile;

    /* we're not in a dungeon, failed! */
    ASSERT(c->location->context & CTX_DUNGEON, "moveAvatarInDungeon() called outside of dungeon, failed!");

    /* you must turn first! */
    if (!advancing && !retreating) {
        c->saveGame->orientation = realDir;
        event.result = MOVE_TURNED;
        return;
    }

    /* figure out our new location */
    newCoords = c->location->coords;
    map_move(newCoords, realDir, c->location->map);

    tile = c->location->map->tileTypeAt(newCoords, WITH_OBJECTS);

    /* see if we moved off the map (really, this should never happen in a dungeon) */
    if (MAP_IS_OOB(c->location->map, newCoords)) {
        event.result = (MoveResult)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);
        return;
    }

    if (!collisionOverride) {
        int movementMask = c->location->map->getValidMoves(c->location->coords, c->party->getTransport());

        if (advancing && !tile->canWalkOn(DIR_ADVANCE))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);
        else if (retreating && !tile->canWalkOn(DIR_RETREAT))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);

        if (!DIR_IN_MASK(realDir, movementMask)) {
            event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
            return;
        }
    }

    /* move succeeded */
    c->location->coords = newCoords;

    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves an object on the map according to its movement behavior
 * Returns 1 if the object was moved successfully, 0 if slowed,
 * tile direction changed, or object simply cannot move
 * (fixed objects, nowhere to go, etc.)
 */
int moveObject(Map *map, Creature *obj, const Coords& avatar) {
    int dirmask;
    Direction dir = DIR_NONE;
    Coords new_coords = obj->coords;
    int slowed = 0;

    /* determine a direction depending on the object's movement behavior */
    switch (obj->movement) {
    case MOVEMENT_FIXED:
    case MOVEMENT_FOLLOW_PAUSE:
        break;

    case MOVEMENT_WANDER:
        /* World map wandering creatures always move, whereas
           town creatures that wander sometimes stay put */
        if (map->isWorldMap() || xu4_random(2) == 0)
            dir = dirRandomDir(map->getValidMoves(new_coords, obj->tile));
        break;

    case MOVEMENT_FOLLOW_AVATAR:
        if (! map->isWorldMap() && xu4_random(2))
            return 0;
        // Fall through...

    case MOVEMENT_ATTACK_AVATAR:
        dirmask = map->getValidMoves(new_coords, obj->tile);

        if (obj->tile.getTileType()->isPirateShip())
            dir = map_pathForward(new_coords, avatar, dirmask,
                                  obj->tile.getDirection(), map);
        else
            dir = map_pathTo(new_coords, avatar, dirmask, true, map);
        break;
    }

    /* now, get a new x and y for the object */
    if (dir)
        map_move(new_coords, dir, map);
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */
    SlowedType slowedType = SLOWED_BY_TILE;
    if (obj->objType == Object::CREATURE)
        slowedType = obj->getSlowedType();

    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileTypeAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(obj->tile.getDirection());
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }

    obj->prevCoords = obj->coords;

    /* see if the object needed to turn instead of move */
    if (obj->setDirection(dir))
        return 0;

    /* was the object slowed? */
    if (slowed)
        return 0;

    /**
     * Set the new coordinates
     */
    if (! (new_coords == obj->coords) &&
        ! MAP_IS_OOB(map, new_coords))
    {
        obj->coords = new_coords;
    }
    return 1;
}

/**
 * Moves an object in combat according to its chosen combat action
 */
int moveCombatObject(int act, Map *map, Creature *obj, const Coords& target) {
    Coords new_coords = obj->coords;
    int valid_dirs = map->getValidMoves(new_coords, obj->tile);
    Direction dir;
    CombatAction action = (CombatAction)act;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    /* fixed objects cannot move */
    if (obj->movement == MOVEMENT_FIXED)
        return 0;

    if (action == CA_FLEE) {
        /* run away from our target instead! */
        dir = map_pathAway(new_coords, target, valid_dirs);

    } else {
        ASSERT(action == CA_ADVANCE, "action must be CA_ADVANCE or CA_FLEE");
        // If they're not fleeing, make sure they don't flee on accident
        if (new_coords.x == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        else if (new_coords.x >= (signed)(map->width - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (new_coords.y == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        else if (new_coords.y >= (signed)(map->height - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);

        dir = map_pathTo(new_coords, target, valid_dirs);
    }

    if (dir)
        map_move(new_coords, dir, c->location->map);
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */
    if (obj->objType == Object::CREATURE)
        slowedType = obj->getSlowedType();

    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileTypeAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(obj->tile.getDirection());
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }

    /* if the object wan't slowed... */
    if (!slowed) {
        // Set the new coordinates
        obj->updateCoords(new_coords);
        return 1;
    }

    return 0;
}

/**
 * Moves a party member during combat screens
 */
static void movePartyMember(MoveEvent &event) {
    CombatController *ct = dynamic_cast<CombatController *>(xu4.eventHandler->getController());
    CombatMap *cm = getCombatMap();
    int member = ct->getFocus();
    Coords newCoords;
    PartyMemberVector *party = ct->getParty();

    event.result = MOVE_SUCCEEDED;

    /* find our new location */
    newCoords = (*party)[member]->coords;
    map_move(newCoords, event.dir, c->location->map);

    if (MAP_IS_OOB(c->location->map, newCoords)) {
        bool sameExit = (!cm->isDungeonRoom() || (ct->getExitDir() == DIR_NONE) || (event.dir == ct->getExitDir()));
        if (sameExit) {
            /* if in a win-or-lose battle and not camping, then it can be bad to flee while healthy */
            if (ct->isWinOrLose() && !ct->isCamping()) {
                /* A fully-healed party member fled from an evil creature :( */
                if (ct->getCreature() && ct->getCreature()->isEvil() &&
                    c->party->member(member)->getHp() == c->party->member(member)->getMaxHp())
                    c->party->adjustKarma(KA_HEALTHY_FLED_EVIL);
            }

            ct->setExitDir(event.dir);
            c->location->map->removeObject((*party)[member]);
            (*party)[member] = NULL;
            event.result = (MoveResult)(MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE | MOVE_SUCCEEDED | MOVE_END_TURN);
            return;
        }
        else {
            event.result = (MoveResult)(MOVE_MUST_USE_SAME_EXIT | MOVE_END_TURN);
            return;
        }
    }

    int movementMask = c->location->map->getValidMoves((*party)[member]->coords, (*party)[member]->tile);
    if (!DIR_IN_MASK(event.dir, movementMask)) {
        event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
        return;
    }

    /* is the party member slowed? */
    if (!slowedByTile(c->location->map->tileTypeAt(newCoords, WITHOUT_OBJECTS)))
    {
        /* move succeeded */
        (*party)[member]->updateCoords(newCoords);

        /* handle dungeon room triggers */
        if (cm->isDungeonRoom()) {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(c->location->prev->map);
            int i;
            Trigger *triggers = dungeon->rooms[dungeon->currentRoom].triggers;

            for (i = 0; i < 4; i++) {
                /*const Creature *m = creatures.getByTile(triggers[i].tile);*/

                /* FIXME: when a creature is created by a trigger, it can be created over and over and over...
                   how do we fix this? In the c64 version is appears that such triggers (on the world map)
                   wipe the creature table and replace it with the triggered creatures. Thus, retriggering
                   it will reset the creatures.
                   */
                Coords trigger(triggers[i].x, triggers[i].y, c->location->coords.z);

                /* see if we're on a trigger */
                if (newCoords == trigger) {
                    Coords change1(triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z),
                              change2(triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z);

                    /**
                     * Remove any previous annotations placed at our target coordinates
                     */
                    AnnotationList& alist = c->location->map->annotations;
                    alist.removeAllAt(change1);
                    alist.removeAllAt(change2);

                    /* change the tiles! */
                    if (change1.x || change1.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z);
                        else*/ alist.add(change1, triggers[i].tile, false, true);
                    }
                    if (change2.x || change2.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z);
                        else*/ alist.add(change2, triggers[i].tile, false, true);
                    }
                }
            }
        }
    }
    else {
        event.result = (MoveResult)(MOVE_SLOWED | MOVE_END_TURN);
        return;
    }
}

/**
 * Add a new location to the stack, or
 * start a new stack if 'prev' is NULL
 */
Location::Location(const Coords& coords, Map *map, int viewmode,
        LocationContext ctx, TurnController *turnCompleter, Location *prev) {

    this->coords = coords;
    this->map = map;
    this->viewMode = viewmode;
    this->context = ctx;
    this->turnCompleter = turnCompleter;

    // Push location onto the stack.
    this->prev = prev;
}

/**
 * Pop a location from the stack and free the memory
 */
void locationFree(Location **stack) {
    Location* loc = *stack;
    *stack = loc->prev;
    loc->prev = NULL;

    delete loc;
}

/**
 * Append the entire stack of objects at the given location to a vector.
 */
void Location::getTilesAt(std::vector<MapTile>& tiles,
                          const Coords& coords, bool& focus) {
    const Object *obj = map->objectAt(coords);
    const Creature *m = dynamic_cast<const Creature *>(obj);
    focus = false;

    bool avatar = this->coords == coords;

    /* Do not return objects for VIEW_GEM mode, show only the avatar and tiles */
    if (viewMode == VIEW_GEM && (!xu4.settings->enhancements || !xu4.settings->enhancementsOptions.peerShowsObjects)) {
        // When viewing a gem, always show the avatar regardless of whether or not
        // it is shown in our normal view
        if (avatar)
            tiles.push_back(c->party->getTransport());
        else
            tiles.push_back(MapTile(map->getTileFromData(coords)));
    }

    /* Add the avatar to gem view */
    if (avatar && viewMode == VIEW_GEM)
        tiles.push_back(c->party->getTransport());

    /* Add visual-only annotations to the list */
    std::list<Annotation *> annot = map->annotations.ptrsToAllAt(coords);
    std::list<Annotation *>::const_iterator i;
    for (i = annot.begin(); i != annot.end(); i++) {
        if ((*i)->visualOnly)
        {
            tiles.push_back((*i)->tile);

            /* If this is the first cover-up annotation,
             * everything underneath it will be invisible,
             * so stop here
             */
            if ((*i)->coverUp)
                return;
        }
    }

    /* then the avatar is drawn (unless on a ship) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext != TRANSPORT_SHIP) && avatar)
        //tiles.push_back(map->tileset->getByName("avatar")->id);
        tiles.push_back(c->party->getTransport());

    /* then camouflaged creatures that have a disguise */
    if (obj && (obj->objType == Object::CREATURE) &&
        ! obj->visible && m->getCamouflageTile()) {
        focus = focus || obj->focused;
        tiles.push_back(map->tileset->getByName(m->getCamouflageTile())->getId());
    }
    /* then visible creatures and objects */
    else if (obj && obj->visible) {
        focus = focus || obj->focused;
        MapTile visibleTile = obj->tile;
        //Sleeping creatures and persons have their animation frozen
        if (m && m->isAsleep())
            visibleTile.freezeAnimation = true;
        else if (obj->animId != ANIM_UNUSED)
            visibleTile.frame = anim_valueI(&xu4.eventHandler->flourishAnim,
                                            obj->animId);
        tiles.push_back(visibleTile);
    }

    /* then the party's ship (because twisters and whirlpools get displayed on top of ships) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext == TRANSPORT_SHIP) && avatar)
        tiles.push_back(c->party->getTransport());

    /* then permanent annotations */
    for (i = annot.begin(); i != annot.end(); i++) {
        if (!(*i)->visualOnly) {
            tiles.push_back((*i)->tile);

            /* If this is the first cover-up annotation,
             * everything underneath it will be invisible,
             * so stop here
             */
            if ((*i)->coverUp)
                return;
        }
    }

    /* finally the base tile */
    MapTile terrainTile(map->getTileFromData(coords));
    tiles.push_back(terrainTile);

    /* But if the base tile requires a background, we must find it */
    const Tile* tileType = terrainTile.getTileType();
    if (tileType->isLandForeground() ||
        tileType->isWaterForeground() ||
        tileType->isLivingObject()) {
        tiles.push_back(getReplacementTile(coords, tileType));
    }
}


/**
 * Finds a valid replacement tile for the given location, using surrounding tiles
 * as guidelines to choose the new tile.  The new tile will only be chosen if it
 * is marked as a valid replacement (or waterReplacement) tile in tiles.xml.  If a valid replacement
 * cannot be found, it returns a "best guess" tile.
 */
TileId Location::getReplacementTile(const Coords& atCoords, const Tile * forTile) {
    std::map<TileId, int> validMapTileCount;

    const static int dirs[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    const static int dirs_per_step = sizeof(dirs) / sizeof(*dirs);
    int loop_count = 0;

    std::set<Coords> searched;
    std::list<Coords> searchQueue;

    //Pathfinding to closest traversable tile with appropriate replacement properties.
    //For tiles marked water-replaceable, pathfinding includes swimmables.
    searchQueue.push_back(atCoords);
    do
    {
        Coords currentStep = searchQueue.front();
        searchQueue.pop_front();

        searched.insert(currentStep);

        for (int i = 0; i < dirs_per_step; i++)
        {
            Coords newStep(currentStep);
            map_move(newStep, dirs[i][0], dirs[i][1], map);

            Tile const * tileType = map->tileTypeAt(newStep,WITHOUT_OBJECTS);

            if (!tileType->isOpaque()) {
                //if (searched.find(newStep) == searched.end()) -- the find mechanism doesn't work.
                searchQueue.push_back(newStep);
            }

            if ((tileType->isReplacement() && (forTile->isLandForeground() || forTile->isLivingObject())) ||
                (tileType->isWaterReplacement() && forTile->isWaterForeground()))
            {
                std::map<TileId, int>::iterator validCount = validMapTileCount.find(tileType->getId());

                if (validCount == validMapTileCount.end())
                {
                    validMapTileCount[tileType->getId()] = 1;
                }
                else
                {
                    validMapTileCount[tileType->getId()]++;
                }
            }
        }

        if (validMapTileCount.size() > 0)
        {
            std::map<TileId, int>::iterator itr = validMapTileCount.begin();

            TileId winner = itr->first;
            int score = itr->second;

            while (++itr != validMapTileCount.end())
            {
                if (score < itr->second)
                {
                    score = itr->second;
                    winner = itr->first;
                }
            }

            return winner;
        }
        /* loop_count is an ugly hack to temporarily fix infinite loop */
    } while (++loop_count < 128 && searchQueue.size() > 0 && searchQueue.size() < 64);

    /* couldn't find a tile, give it the classic default */
    return map->tileset->getByName(Tile::sym.brickFloor)->getId();
}

/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
int Location::getCurrentPosition(Coords* pos) {
    if (context & CTX_COMBAT) {
        CombatController *cc = dynamic_cast<CombatController *>(xu4.eventHandler->getController());
        PartyMemberVector *party = cc->getParty();
        *pos = (*party)[cc->getFocus()]->coords;
    }
    else
        *pos = coords;

    return 1;
}

MoveResult Location::move(Direction dir, bool userEvent) {
    MoveEvent event(dir, userEvent);
    event.location = this;
    switch (map->type) {

    case Map::DUNGEON:
        moveAvatarInDungeon(event);
        break;

    case Map::COMBAT:
        movePartyMember(event);
        break;

    default:
        moveAvatar(event);
        break;
    }

    gs_emitMessage(SENDER_LOCATION, &event);
    return event.result;
}
