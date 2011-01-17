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

void DungeonView::drawInDungeon(Tile *tile, int distance, Direction orientation, bool tiledWall) {
    Image *baseTileImage, *scaled;
    const static int nscale[] = { 11, 9, 5, 2 }, doffset[] = { 88 , 88, 88, 88 };
    const static int lscale[] = { 22, 18, 10, 4};
    const int *dscale = tiledWall ? lscale : nscale;

    // create our animated version of the tile
    if (tile->getAnim()) {
        MapTile mt = tile->id;
        tile->getAnim()->draw(animated, tile, mt, orientation);
        baseTileImage = Image::duplicate(animated);
    }
    else
    	baseTileImage = Image::duplicate(tile->getImage());

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 1)
        scaled = screenScaleDown(baseTileImage, 2);
    else
        scaled = screenScale(baseTileImage, dscale[distance] / 2, 1, 1);


    if (tiledWall) {
    	int i_x = SCALED((VIEWPORT_W * tileWidth / 2) + this->x) - (scaled->width() / 2);
    	int i_y = SCALED((VIEWPORT_H * tileHeight / 2) + this->y) - (scaled->height() / 2);
    	int f_x = i_x + scaled->width();
    	int f_y = i_y + scaled->height();
    	int d_x = baseTileImage->width();
    	int d_y = baseTileImage->height();

    	for (int x = i_x; x < f_x; x+=d_x)
    		for (int y = i_y; y < f_y; y+=d_y)
    			//TODO Add a check for the rectangle w,h fields to deal with (currently non-existent) case where the scaling isn't a direct multiple of the tile width
				baseTileImage->drawSubRect(x,y,0,0,f_x,f_y);
    }
    else {
    scaled->drawSubRect(SCALED((VIEWPORT_W * tileWidth / 2) + this->x) - (scaled->width() / 2),
                        SCALED(doffset[distance] + this->y),
                        0,
                        0,
                        scaled->width(),
                        scaled->height());
    }
    delete scaled;
    delete baseTileImage;
}

