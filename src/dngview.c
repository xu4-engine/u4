/*
 * $Id$
 */

#include <stdio.h>
#include "u4.h"

#include "dngview.h"

#include "context.h"
#include "dungeon.h"
#include "location.h"
#include "savegame.h"
#include "ttype.h"

unsigned char dungeonViewGetVisibleTile(int fwd, int side) {
    int x, y;
    unsigned char tile;    

    switch (c->saveGame->orientation) {
    case DIR_WEST:
        x = c->location->x - fwd;
        y = c->location->y - side;
        break;

    case DIR_NORTH:
        x = c->location->x + side;
        y = c->location->y - fwd;
        break;

    case DIR_EAST:
        x = c->location->x + fwd;
        y = c->location->y + side;
        break;

    case DIR_SOUTH:
        x = c->location->x - side;
        y = c->location->y + fwd;
        break;
    }
    if (MAP_IS_OOB(c->location->map, x, y)) {        
        while (x < 0)
            x += c->location->map->width;
        while (y < 0)
            y += c->location->map->height;
        while (x >= (int)c->location->map->width)
            x -= c->location->map->width;
        while (y >= (int)c->location->map->height)
            y -= c->location->map->height;
    }
    tile = (*c->location->tileAt)(c->location->map, x, y, c->location->z, WITH_OBJECTS);

    return tile;
}

DungeonGraphicType dungeonViewTileToGraphic(unsigned char tile) {
    DungeonToken token = dungeonTokenForTile(tile);    

    switch (token) {
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
        return DNGGRAPHIC_TILE;
    }
}
