/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "u4.h"

#include "dngview.h"

#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "savegame.h"
#include "tile.h"

MapTileList dungeonViewGetTiles(int fwd, int side) {    
    int focus;
    MapCoords coords = c->location->coords;

    switch (c->saveGame->orientation) {
    case DIR_WEST:
        coords.x -= fwd;
        coords.y -= side;
        break;

    case DIR_NORTH:
        coords.x += side;
        coords.y -= fwd;
        break;

    case DIR_EAST:
        coords.x += fwd;
        coords.y += side;
        break;

    case DIR_SOUTH:
        coords.x -= side;
        coords.y += fwd;
        break;
    
    case DIR_ADVANCE:
    case DIR_RETREAT:
    default:
        ASSERT(0, "Invalid dungeon orientation");        
    }
    
    // Wrap the coordinates if necessary
    coords.wrap(c->location->map);

    return locationTilesAt(c->location, coords, &focus);
}

DungeonGraphicType dungeonViewTilesToGraphic(MapTileList tiles) {
    MapTile tile = tiles->front();
    DungeonToken token;

    /* 
     * check if the dungeon tile has an annotation or object on top
     * (always displayed as a tile, unless a ladder)
     */
    if (tiles->size() > 1) {
        if (tile == Tile::get(LADDERUP_TILE)->id)
            return DNGGRAPHIC_LADDERUP;
        else if (tile == Tile::get(LADDERDOWN_TILE)->id)
            return DNGGRAPHIC_LADDERDOWN;
        else
            return DNGGRAPHIC_BASETILE;        
    }

    /* 
     * if not an annotation or object, then the tile is a dungeon
     * token 
     */
    token = dungeonTokenForTile(tile);

    switch (token) {
    case DUNGEON_TRAP:
    case DUNGEON_CORRIDOR:
        return DNGGRAPHIC_NONE;
    case DUNGEON_WALL:
    case DUNGEON_SECRET_DOOR:
        return DNGGRAPHIC_WALL;
    case DUNGEON_ROOM:
    case DUNGEON_DOOR:
        return DNGGRAPHIC_DOOR;
    case DUNGEON_LADDER_UP:
        return DNGGRAPHIC_LADDERUP;
    case DUNGEON_LADDER_DOWN:
        return DNGGRAPHIC_LADDERDOWN;
    
    default:
        return DNGGRAPHIC_DNGTILE;
    }
}
