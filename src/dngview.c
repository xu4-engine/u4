/*
 * $Id$
 */

#include <stdio.h>

#include "u4.h"
#include "context.h"
#include "savegame.h"
#include "ttype.h"
#include "dngview.h"
#include "location.h"
#include "ttype.h"


unsigned char dungeonViewGetVisibleTile(int fwd, int side) {
    int x, y;
    unsigned char tile;

    c->saveGame->orientation = DIR_EAST;

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
        while (x >= c->location->map->width)
            x -= c->location->map->width;
        while (y >= c->location->map->height)
            y -= c->location->map->height;
    }
    tile = mapTileAt(c->location->map, x, y, c->location->z);

    printf("tile (%d, %d) = %d\n", fwd, side, tile);

    switch (tile & 0xF0) {
    case 0x00:
    case 0x80:
        return BRICKFLOOR_TILE;
    case 0x10:
        return 0x1b;
    case 0x20:
        return 0x1c;
    case 0x40:
        return tileGetChestBase();
    case 0x50:
    case 0x60:
        return BRICKFLOOR_TILE; /* FIXME */
    case 0x70:
        return MAGICFLASH_TILE;
    case 0x90:
        return 0;
    case 0xA0:
        return LIGHTNINGFIELD_TILE;
    case 0xB0:
        return BRICKFLOOR_TILE; /* FIXME: altar */
    case 0xC0:
    case 0xD0:
        return 0x3b;

    case 0xE0:
    case 0xF0:
        return 0x7F;
    default:
        return BLACK_TILE;
    }
}

