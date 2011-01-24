/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <list>
#include <map>

#include "location.h"

#include "annotation.h"
#include "context.h"
#include "combat.h"
#include "creature.h"
#include "event.h"
#include "game.h"
#include "map.h"
#include "object.h"
#include "savegame.h"
#include "settings.h"
#include "tileset.h"

Location *locationPush(Location *stack, Location *loc);
Location *locationPop(Location **stack);

/**
 * Add a new location to the stack, or
 * start a new stack if 'prev' is NULL
 */
Location::Location(MapCoords coords, Map *map, int viewmode, LocationContext ctx,
                   TurnCompleter *turnCompleter, Location *prev) {

    this->coords = coords;
    this->map = map;
    this->viewMode = viewmode;
    this->context = ctx;
    this->turnCompleter = turnCompleter;

    locationPush(prev, this);
}

/**
 * Return the entire stack of objects at the given location.
 */
std::vector<MapTile> Location::tilesAt(MapCoords coords, bool &focus) {
    std::vector<MapTile> tiles;
    std::list<Annotation *> a = map->annotations->ptrsToAllAt(coords);
    std::list<Annotation *>::iterator i;
    Object *obj = map->objectAt(coords);
    Creature *m = dynamic_cast<Creature *>(obj);
    focus = false;

    bool avatar = this->coords == coords;

    /* Do not return objects for VIEW_GEM mode, show only the avatar and tiles */
    if (viewMode == VIEW_GEM && (!settings.enhancements || !settings.enhancementsOptions.peerShowsObjects)) {        
        // When viewing a gem, always show the avatar regardless of whether or not
        // it is shown in our normal view
        if (avatar)
            tiles.push_back(c->party->getTransport());
        else             
            tiles.push_back(*map->getTileFromData(coords));

        return tiles;
    }

    /* Add the avatar to gem view */
    if (avatar && viewMode == VIEW_GEM)
        tiles.push_back(c->party->getTransport());
    
    /* Add visual-only annotations to the list */
    for (i = a.begin(); i != a.end(); i++) {
        if ((*i)->isVisualOnly())        
        {
            tiles.push_back((*i)->getTile());

            /* If this is the first cover-up annotation,
			 * everything underneath it will be invisible,
			 * so stop here
			 */
			if ((*i)->isCoverUp())
				return tiles;
        }
    }

    /* then the avatar is drawn (unless on a ship) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext != TRANSPORT_SHIP) && avatar)
        //tiles.push_back(map->tileset->getByName("avatar")->id);
        tiles.push_back(c->party->getTransport());

    /* then camouflaged creatures that have a disguise */
    if (obj && (obj->getType() == Object::CREATURE) && !obj->isVisible() && (!m->getCamouflageTile().empty())) {
        focus = focus || obj->hasFocus();
        tiles.push_back(map->tileset->getByName(m->getCamouflageTile())->id);
    }
    /* then visible creatures and objects */
    else if (obj && obj->isVisible()) {
        focus = focus || obj->hasFocus();
        MapTile visibleCreatureAndObjectTile = obj->getTile();
		//Sleeping creatures and persons have their animation frozen
		if (m && m->isAsleep())
			visibleCreatureAndObjectTile.freezeAnimation = true;
        tiles.push_back(visibleCreatureAndObjectTile);
    }

    /* then the party's ship (because twisters and whirlpools get displayed on top of ships) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext == TRANSPORT_SHIP) && avatar)
        tiles.push_back(c->party->getTransport());

    /* then permanent annotations */
    for (i = a.begin(); i != a.end(); i++) {
        if (!(*i)->isVisualOnly()) {
            tiles.push_back((*i)->getTile());

            /* If this is the first cover-up annotation,
             * everything underneath it will be invisible,
             * so stop here
             */
            if ((*i)->isCoverUp())
            	return tiles;
        }
    }

    /* finally the base tile */
    MapTile tileFromMapData = *map->getTileFromData(coords);
    const Tile * tileType = map->getTileFromData(coords)->getTileType();
    if (tileType->isLivingObject())
    {
    	//This animation should be frozen because a living object represented on the map data is usually a statue of a monster or something
    	tileFromMapData.freezeAnimation = true;
    }
	tiles.push_back(tileFromMapData);



    if (tileType->isLandForeground()	||
    	tileType->isWaterForeground()	||
    	tileType->isLivingObject())
    	tiles.push_back(getReplacementTile(coords, tileType));

    return tiles;
}


/**
 * Finds a valid replacement tile for the given location, using surrounding tiles
 * as guidelines to choose the new tile.  The new tile will only be chosen if it
 * is marked as a valid replacement (or waterReplacement) tile in tiles.xml.  If a valid replacement
 * cannot be found, it returns a "best guess" tile.
 */
TileId Location::getReplacementTile(MapCoords atCoords, const Tile * forTile) {
    std::map<TileId, int> validMapTileCount;

    int surround[][2] = {{-1,-1},{-1,0},{-1,1},
    					{ 0,-1},   	   { 0,1},
    					{ 1,-1},{ 1,0},{ 1,1}};

    //Fan out into 8 surrounding directions and find the most appropriate tile
    //Todo, employ pathfinding as the shortest walkable path (or swimable for watery tiles) usually denotes enclosed rooms. See "Hawkwind" for a good example of this algorithm failing
	for (int d = 1; d < 5; d++)
	{
		for (int i = 0; i < 8; i++)
		{
			MapCoords new_c(atCoords);
			new_c.move(surround[i][0] * d, surround[i][1] * d, map);

		    if (MAP_IS_OOB(map, atCoords))
		    	continue;

			Tile const * tileType = map->tileAt(new_c, WITHOUT_OBJECTS)->getTileType();

			if ((tileType->isReplacement() && (forTile->isLandForeground() || forTile->isLivingObject())) ||
				(tileType->isWaterReplacement() && forTile->isWaterForeground()))
			{
				std::map<TileId, int>::iterator validCount = validMapTileCount.find(tileType->id);

				if (validCount == validMapTileCount.end())
				{
					validMapTileCount[tileType->id] = 1;
				}
				else
				{
					validMapTileCount[tileType->id]++;
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
	}

    /* couldn't find a tile, give it the sad default */
    return map->tileset->getByName("brick_floor")->id;
}

/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
int Location::getCurrentPosition(MapCoords *coords) {
    if (context & CTX_COMBAT) {
        CombatController *cc = dynamic_cast<CombatController *>(eventHandler->getController());
        PartyMemberVector *party = cc->getParty();
        *coords = (*party)[cc->getFocus()]->getCoords();    
    }
    else
        *coords = this->coords;

    return 1;
}

MoveResult Location::move(Direction dir, bool userEvent) {
    MoveEvent event(dir, userEvent);
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

    setChanged();
    notifyObservers(event);

    return event.result;
}


/**
 * Pop a location from the stack and free the memory
 */
void locationFree(Location **stack) {
    delete locationPop(stack);
}

/**
 * Push a location onto the stack
 */
Location *locationPush(Location *stack, Location *loc) {
    loc->prev = stack;
    return loc;
}

/**
 * Pop a location off the stack
 */
Location *locationPop(Location **stack) {
    Location *loc = *stack;
    *stack = (*stack)->prev;
    loc->prev = NULL;
    return loc;
}
