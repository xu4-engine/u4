/*
 * $Id$
 */

#include <stdio.h>

#include "u4.h"
#include "context.h"
#include "savegame.h"
#include "ttype.h"
#include "dngview.h"
#include "map.h"
#include "ttype.h"


unsigned char dungeonViewGetVisibleTile(int fwd, int side) {
    int x, y;
    unsigned char tile;

    c->saveGame->orientation = DIR_EAST;

    switch (c->saveGame->orientation) {
    case DIR_WEST:
        x = c->saveGame->x - fwd;
        y = c->saveGame->y - side;
        break;

    case DIR_NORTH:
        x = c->saveGame->x + side;
        y = c->saveGame->y - fwd;
        break;

    case DIR_EAST:
        x = c->saveGame->x + fwd;
        y = c->saveGame->y + side;
        break;

    case DIR_SOUTH:
        x = c->saveGame->x - side;
        y = c->saveGame->y + fwd;
        break;
    }
    if (MAP_IS_OOB(c->map, x, y)) {
        while (x < 0)
            x += c->map->width;
        while (y < 0)
            y += c->map->height;
        while (x >= c->map->width)
            x -= c->map->width;
        while (y >= c->map->height)
            y -= c->map->height;
    }
    tile = mapTileAt(c->map, x, y, c->saveGame->dnglevel);

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

