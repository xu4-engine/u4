/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <list>

#include "location.h"

#include "annotation.h"
#include "context.h"
#include "combat.h"
#include "creature.h"
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
                   FinishTurnCallback finishTurnCallback, Location *prev) {

    this->coords = coords;
    this->map = map;
    this->viewMode = viewmode;
    this->context = ctx;
    this->finishTurn = finishTurnCallback;

    locationPush(prev, this);
}

/**
 * Returns the visible tile at the given point on a map.  This
 * includes visual-only annotations like attack icons.
 */
MapTile *Location::visibleTileAt(MapCoords coords, bool &focus) {

    /* get the stack of tiles and take the top tile */
    std::vector<MapTile *> tiles = tilesAt(coords, focus);

    return tiles.front();
}

/**
 * Return the entire stack of objects at the given location.
 */
std::vector<MapTile *> Location::tilesAt(MapCoords coords, bool &focus) {
    std::vector<MapTile *> tiles;
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
            tiles.push_back(&c->party->transport);
        else             
            tiles.push_back(map->getTileFromData(coords));

        return tiles;
    }

    /* Add the avatar to gem view */
    if (avatar && viewMode == VIEW_GEM)
        tiles.push_back(&c->party->transport);
    
    /* Add visual-only annotations to the list */
    for (i = a.begin(); i != a.end(); i++) {
        if ((*i)->isVisualOnly())        
            tiles.push_back(&(*i)->getTile());
    }

    /* then the avatar is drawn (unless on a ship) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext != TRANSPORT_SHIP) && avatar)
        tiles.push_back(&c->party->transport);

    /* then camouflaged creatures that have a disguise */
    if (obj && (obj->getType() == Object::CREATURE) && !obj->isVisible() && (m->camouflageTile.id > 0)) {
        focus = focus || obj->hasFocus();
        tiles.push_back(&m->camouflageTile);
    }
    /* then visible creatures */
    else if (obj && (obj->getType() != Object::UNKNOWN) && obj->isVisible()) {
        focus = focus || obj->hasFocus();
        tiles.push_back(&obj->getTile());
    }

    /* then other visible objects */
     if (obj && obj->isVisible()) {
        focus = focus || obj->hasFocus();
        tiles.push_back(&obj->getTile());
    }

    /* then the party's ship (because twisters and whirlpools get displayed on top of ships) */
    if ((map->flags & SHOW_AVATAR) && (c->transportContext == TRANSPORT_SHIP) && avatar)
        tiles.push_back(&c->party->transport);

    /* then permanent annotations */
    for (i = a.begin(); i != a.end(); i++) {
        if (!(*i)->isVisualOnly())
            tiles.push_back(&(*i)->getTile());
    }

    /* finally the base tile */
    tiles.push_back(map->getTileFromData(coords));

    return tiles;
}


/**
 * Finds a valid replacement tile for the given location, using surrounding tiles
 * as guidelines to choose the new tile.  The new tile will only be chosen if it
 * is marked as a valid replacement tile in tiles.xml.  If a valid replacement
 * cannot be found, it returns a "best guess" tile.
 */
MapTile Location::getReplacementTile(MapCoords coords) {
    Direction d;

    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        MapCoords new_c = coords;        
        MapTile newTile;

        new_c.move(d, map);        
        newTile = *map->tileAt(new_c, WITHOUT_OBJECTS);

        /* make sure the tile we found is a valid replacement */
        if (newTile.isReplacement())
            return newTile;
    }

    /* couldn't find a tile, give it our best guess */
    return map->tileset->getByName("brick_floor")->id;
}

/**
 * Returns the current coordinates of the location given:
 *     If in combat - returns the coordinates of party member with focus
 *     If elsewhere - returns the coordinates of the avatar
 */
int Location::getCurrentPosition(MapCoords *coords) {
    if (context & CTX_COMBAT) {
        PartyMemberVector *party = c->combat->getParty();
        *coords = (*party)[c->combat->getFocus()]->getCoords();    
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
