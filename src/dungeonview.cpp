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
#include "error.h"

DungeonView::DungeonView(int x, int y, int columns, int rows) : TileView(x, y, rows, columns) {
}

DungeonView::DungeonView(int x, int y, int columns, int rows, const string &tileset) : TileView(x, y, rows, columns, tileset) {
}

void DungeonView::drawInDungeon(Tile *tile, int x_offset, int distance, Direction orientation, bool tiledWall) {
	Image *scaled;

  	const static int nscale_vga[] = { 12, 8, 4, 2, 1};
    const static int nscale_ega[] = { 8, 4, 2, 1, 0};

	const int lscale_vga[] = { 22, 18, 10, 4, 1};
	const int lscale_ega[] = { 22, 14, 6, 3, 1};

    const int * lscale;
    const int * nscale;
    int offset_multiplier = 0;
    int offset_adj = 0;
    if (settings.videoType != "EGA")
    {
    	lscale = & lscale_vga[0];
    	nscale = & nscale_vga[0];
    	offset_multiplier = 1;
    	offset_adj = 2;
    }
    else
    {
    	lscale = & lscale_ega[0];
    	nscale = & nscale_ega[0];
    	offset_adj = 1;
    	offset_multiplier = 4;
    }

    const int *dscale = tiledWall ? lscale : nscale;

    //Clear scratchpad and set a background colour
    animated->initializeToBackgroundColour();
    //Put tile on animated scratchpad
    if (tile->getAnim()) {
        MapTile mt = tile->id;
        tile->getAnim()->draw(animated, tile, mt, orientation);
    }
    else
    {
        tile->getImage()->drawOn(animated, 0, 0);
    }
    animated->makeBackgroundColourTransparent();
    //This process involving the background colour is only required for drawing in the dungeon.
    //It will not play well with semi-transparent graphics.

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    if (dscale[distance] == 0)
		return;
    else if (dscale[distance] == 1)
        scaled = screenScaleDown(animated, 2);
    else
    {
        scaled = screenScale(animated, dscale[distance] / 2, 1, 0);
    }

    if (tiledWall) {
    	int i_x = SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x) - (scaled->width() / 2.0);
    	int i_y = SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y) - (scaled->height() / 2.0);
    	int f_x = i_x + scaled->width();
    	int f_y = i_y + scaled->height();
    	int d_x = animated->width();
    	int d_y = animated->height();

    	for (int x = i_x; x < f_x; x+=d_x)
    		for (int y = i_y; y < f_y; y+=d_y)
    			animated->drawSubRectOn(this->screen,x,y,0,0,f_x - x,f_y - y);
    }
    else {
    	int y_offset = std::max(0,(dscale[distance] - offset_adj) * offset_multiplier);
    	int x = SCALED((VIEWPORT_W * tileWidth / 2.0) + this->x) - (scaled->width() / 2.0);
    	int y = SCALED((VIEWPORT_H * tileHeight / 2.0) + this->y + y_offset) - (scaled->height() / 8.0);

		scaled->drawSubRectOn(	this->screen,
								x,
								y,
								0,
								0,
								SCALED(tileWidth * VIEWPORT_W + this->x) - x ,
								SCALED(tileHeight * VIEWPORT_H + this->y) - y );
    }

    delete scaled;
}

