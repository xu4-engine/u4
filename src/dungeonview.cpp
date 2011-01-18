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

void DungeonView::drawInDungeon(Tile *tile, int x_offset, int distance, Direction orientation, bool tiledWall) {
	Image *baseTileImage, *scaled;

  	const static int nscale_vga[] = { 16, 8, 4, 2, 1};
    const static int nscale_ega[] = { 16, 8, 4, 1, 0};

	const int lscale_vga[] = { 22, 18, 10, 4, 1};
	const int lscale_ega[] = { 22, 14, 6, 3, 1};

    const int * lscale;
    const int * nscale;
    if (settings.videoType == "VGA")
    {
    	lscale = & lscale_vga[0];
    	nscale = & nscale_vga[0];
    }
    else
    {
    	lscale = & lscale_ega[0];
    	nscale = & nscale_ega[0];

    }

    const int *dscale = tiledWall ? lscale : nscale;

    // create our animated version of the tile
	baseTileImage = Image::duplicate(tile->getImage());
    if (tile->getAnim()) {
        MapTile mt = tile->id;
        tile->getAnim()->draw(animated, tile, mt, orientation);
        delete baseTileImage;
        baseTileImage = Image::duplicate(animated);
    }

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 0)
		return;
    else if (dscale[distance] == 1)
        scaled = screenScaleDown(baseTileImage, 2);
    else
        scaled = screenScale(baseTileImage, dscale[distance] / 2, 1, 1);


    if (tiledWall) {
    	baseTileImage->alphaOn();
    	baseTileImage->setTransparentIndex(0);
    	int i_x = SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x) - (scaled->width() / 2.0);
    	int i_y = SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y) - (scaled->height() / 2.0);
    	int f_x = i_x + scaled->width();
    	int f_y = i_y + scaled->height();
    	int d_x = baseTileImage->width();
    	int d_y = baseTileImage->height();

    	for (int x = i_x; x < f_x; x+=d_x)
    		for (int y = i_y; y < f_y; y+=d_y)
    			//TODO Add a check for the rectangle w,h fields to deal with (currently non-existent) case where the scaling isn't a direct multiple of the tile width

    			baseTileImage->drawSubRectOn(this->screen,x,y,0,0,f_x - x,f_y - y);
    }
    else {
    	scaled->alphaOn();
    	scaled->setTransparentIndex(0);

    	int x = SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x) - (scaled->width() / 2.0);
    	int y = SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y) - (scaled->height() / 8.0);

    scaled->drawSubRectOn(	this->screen,
    						x,
    						y,
    						0,
    						0,
    						SCALED(tileWidth * VIEWPORT_W + this->x) - x ,
    						SCALED(tileHeight * VIEWPORT_H + this->y) - y );
    }

    delete scaled;
    delete baseTileImage;
}

