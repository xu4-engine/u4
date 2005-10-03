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
#include "tileset.h"

std::vector<MapTile> dungeonViewGetTiles(int fwd, int side) {    
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

    bool focus;
    return c->location->tilesAt(coords, focus);
}

DungeonGraphicType dungeonViewTilesToGraphic(const std::vector<MapTile> &tiles) {
    MapTile tile = tiles.front();

    static const MapTile corridor = c->location->map->tileset->getByName("brick_floor")->id;
    static const MapTile up_ladder = c->location->map->tileset->getByName("up_ladder")->id;
    static const MapTile down_ladder = c->location->map->tileset->getByName("down_ladder")->id;
    static const MapTile updown_ladder = c->location->map->tileset->getByName("up_down_ladder")->id;

    /* 
     * check if the dungeon tile has an annotation or object on top
     * (always displayed as a tile, unless a ladder)
     */
    if (tiles.size() > 1) {
        if (tile.id == up_ladder.id)
            return DNGGRAPHIC_LADDERUP;
        else if (tile.id == down_ladder.id)
            return DNGGRAPHIC_LADDERDOWN;
        else if (tile.id == updown_ladder.id)            
            return DNGGRAPHIC_LADDERUPDOWN;
        else if (tile.id == corridor.id)
            return DNGGRAPHIC_NONE;
        else
            return DNGGRAPHIC_BASETILE;
    }

    /* 
     * if not an annotation or object, then the tile is a dungeon
     * token 
     */
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    DungeonToken token = dungeon->tokenForTile(tile);

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
    case DUNGEON_LADDER_UPDOWN:        
        return DNGGRAPHIC_LADDERUPDOWN;
    
    default:
        return DNGGRAPHIC_DNGTILE;
    }
}
