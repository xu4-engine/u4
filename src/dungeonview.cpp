/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "debug.h"
#include "dungeonview.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"

DungeonView::DungeonView(int x, int y, int columns, int rows) : TileView(x, y, rows, columns) {
}

DungeonView::DungeonView(int x, int y, int columns, int rows, const string &tileset) : TileView(x, y, rows, columns, tileset) {
}

void DungeonView::drawInDungeon(MapTile *mapTile, int distance, Direction orientation, bool large) {
    Tile *tile = tileset->get(mapTile->id);    
    Image *tmp, *scaled;
    const static int nscale[] = { 8, 4, 2, 1 }, doffset[] = { 96, 96, 88, 88 };
    const static int lscale[] = { 22, 14, 6, 2 };
    const int *dscale = large ? lscale : nscale;

    // create our animated version of the tile
    if (tile->anim) {
        tile->anim->draw(animated, tile, mapTile, orientation);
        tmp = Image::duplicate(animated);
    }
    else
        tmp = Image::duplicate(tile->getImage());

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 1)
        scaled = screenScaleDown(tmp, 2);
    else
        scaled = screenScale(tmp, dscale[distance] / 2, 1, 1);

    scaled->drawSubRect(SCALED((VIEWPORT_W * tileWidth / 2) + this->x) - (scaled->width() / 2),
                        large 
                        ? SCALED((VIEWPORT_H * tileHeight / 2) + this->y) - (scaled->height() / 2)
                        : SCALED(doffset[distance] + this->y),
                        0,
                        0,
                        scaled->width(),
                        scaled->height());

    delete scaled;
    delete tmp;
}

