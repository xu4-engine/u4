/*
 * $Id$
 */

#include <stdio.h>
#include "u4.h"

#include "dngview.h"

#include "context.h"
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
    switch (tile) {
    case WALL_TILE:
    case 73: /* secret door */
        return DNGGRAPHIC_WALL;
    case 59:
    case 72:
        return DNGGRAPHIC_DOOR;
    case LADDERUP_TILE:
        return DNGGRAPHIC_LADDERUP;
    case LADDERDOWN_TILE:
        return DNGGRAPHIC_LADDERDOWN;
    case DEEP_WATER_TILE:    
    case CHEST_TILE:
    case ALTAR_TILE:
    case MAGICFLASH_TILE:
        return DNGGRAPHIC_TILE;
    }

    return DNGGRAPHIC_NONE;
}
