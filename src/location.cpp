/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <list>

#include "location.h"

#include "annotation.h"
#include "context.h"
#include "combat.h"
#include "game.h"
#include "map.h"
#include "creature.h"
#include "object.h"
#include "savegame.h"
#include "tileset.h"

Location *locationPush(Location *stack, Location *loc);
Location *locationPop(Location **stack);

/**
 * Add a new location to the stack, or
 * start a new stack if 'prev' is NULL
 */
Location *locationNew(MapCoords coords, Map *map, int viewmode, LocationContext ctx,
                      FinishTurnCallback finishTurnCallback, MoveCallback moveCallback,
                      Tileset *tileset, Location *prev) {

    Location *newLoc = new Location;

    newLoc->coords = coords;
    newLoc->map = map;
    newLoc->viewMode = viewmode;
    newLoc->context = ctx;
    newLoc->finishTurn = finishTurnCallback;
    newLoc->move = moveCallback;    
    newLoc->tileset = tileset;
    newLoc->activePlayer = -1;

    return locationPush(prev, newLoc);
}

/**
 * Returns the visible tile at the given point on a map.  This
 * includes visual-only annotations like attack icons.
 */
MapTile locationVisibleTileAt(Location *location, MapCoords coords, int *focus) {
    MapTile tile;
    MapTileList tiles;

    /* get the stack of tiles and take the top tile */
    tiles = locationTilesAt(location, coords, focus);
    tile = tiles->front();
    delete tiles;

    return tile;
}

/**
 * Return the entire stack of objects at the given location.
 */
MapTileList locationTilesAt(Location *location, MapCoords coords, int *focus) {    
    MapTileList tiles = new std::list<MapTile>;
    AnnotationList a = location->map->annotations->allAt(coords);    
    AnnotationList::iterator i;
    const Object *obj = location->map->objectAt(coords);
    const Creature *m = dynamic_cast<const Creature *>(obj);
    *focus = 0;

    bool avatar = location->coords == coords;

    /* Do not return objects for VIEW_GEM mode, show only the avatar and tiles */
    if (location->viewMode == VIEW_GEM) {
        if ((location->map->flags & SHOW_AVATAR) && avatar)
            tiles->push_back(c->saveGame->transport);
        else
            tiles->push_back(location->map->getTileFromData(coords));
        return tiles;
    }
    
    /* Add visual-only annotations to the list */
    for (i = a.begin(); i != a.end(); i++) {
        if (i->isVisualOnly())        
            tiles->push_back(i->getTile());
    }

    /* then the avatar is drawn (unless on a ship) */
    if ((location->map->flags & SHOW_AVATAR) && (c->transportContext != TRANSPORT_SHIP) && avatar)
        tiles->push_back(c->saveGame->transport);

    /* then camouflaged creatures that have a disguise */
    if (obj && (obj->getType() == OBJECT_CREATURE) && !obj->isVisible() && (m->camouflageTile > 0)) {
        *focus = *focus || obj->hasFocus();
        tiles->push_back(m->camouflageTile);
    }
    /* then visible creatures */
    else if (obj && (obj->getType() != OBJECT_UNKNOWN) && obj->isVisible()) {
        *focus = *focus || obj->hasFocus();
        tiles->push_back(obj->getTile());
    }

    /* then other visible objects */
     if (obj && obj->isVisible()) {
        *focus = *focus || obj->hasFocus();
        tiles->push_back(obj->getTile());
    }

    /* then the party's ship (because twisters and whirlpools get displayed on top of ships) */
    if ((location->map->flags & SHOW_AVATAR) && (c->transportContext == TRANSPORT_SHIP) && avatar)
        tiles->push_back(c->saveGame->transport);

    /* then permanent annotations */
    for (i = a.begin(); i != a.end(); i++) {
        if (!i->isVisualOnly())
            tiles->push_back(i->getTile());
    }

    /* finally the base tile */
    tiles->push_back(location->map->getTileFromData(coords));

    return tiles;
}


/**
 * Finds a valid replacement tile for the given location, using surrounding tiles
 * as guidelines to choose the new tile.  The new tile will only be chosen if it
 * is marked as a valid replacement tile in tiles.xml.  If a valid replacement
 * cannot be found, it returns a "best guess" tile.
 */
MapTile locationGetReplacementTile(Location *location, MapCoords coords) {
    Direction d;

    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        MapCoords new_c = coords;        
        MapTile newTile;

        new_c.move(d, location->map);        
        newTile = location->map->tileAt(new_c, WITHOUT_OBJECTS);

        /* make sure the tile we found is a valid replacement */
        if (tileIsReplacement(newTile))
            return newTile;
    }

    /* couldn't find a tile, give it our best guess */
    if (location->context & CTX_DUNGEON)
        return 0;
    else
        return (location->context & CTX_COMBAT) ? BRICKFLOOR_1_TILE : BRICKFLOOR_TILE;
}

/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
int locationGetCurrentPosition(Location *location, MapCoords *coords) {
    if (location->context & CTX_COMBAT) {
        PartyMemberVector *party = c->combat->getParty();
        *coords = (*party)[c->combat->getFocus()]->getCoords();    
    }
    else
        *coords = location->coords;

    return 1;
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
