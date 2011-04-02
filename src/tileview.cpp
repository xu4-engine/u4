/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "debug.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "u4.h"

using std::vector;

TileView::TileView(int x, int y, int columns, int rows) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->tileWidth = TILE_WIDTH;
    this->tileHeight = TILE_HEIGHT;
    this->tileset = Tileset::get("base");
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight), false, Image::HARDWARE);
    animated->alphaOn();
}

TileView::TileView(int x, int y, int columns, int rows, const string &tileset) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->tileWidth = TILE_WIDTH;
    this->tileHeight = TILE_HEIGHT;
    this->tileset = Tileset::get(tileset);
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight), false, Image::HARDWARE);
    animated->alphaOn();
}

TileView::~TileView() {
    delete animated;
}

void TileView::reinit() {
    View::reinit();
    tileset = Tileset::get("base");

    //Scratchpad needs to be re-inited if we rescale...
    if (animated)
    {
    	delete animated;
    	animated = NULL;
    }
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight), false, Image::HARDWARE);
    animated->alphaOn();
}

void TileView::loadTile(MapTile &mapTile)
{
	//Not sure if this is wanted. Defeats the purpose of the lazy evaluation? (or does this solve a problem?)
    Tile *tile = tileset->get(mapTile.id);
    tile->getImage();
}

void TileView::drawTile(MapTile &mapTile, bool focus, int x, int y) {
    Tile *tile = tileset->get(mapTile.id);
    Image *image = tile->getImage();

    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

	animated->fillRect(0,0,SCALED(tileWidth),SCALED(tileHeight),0,0,0, 255);
	animated->drawSubRect(SCALED(x * tileWidth + this->x),
						  SCALED(y * tileHeight + this->y),
						  0,
						  0,
						  SCALED(tileWidth),
						  SCALED(tileHeight));

    // draw the tile to the screen
    if (tile->getAnim()) {
        // First, create our animated version of the tile
#ifdef IOS
        animated->clearImageContents();
#endif
        tile->getAnim()->draw(animated, tile, mapTile, DIR_NONE);

        // Then draw it to the screen
        animated->drawSubRect(SCALED(x * tileWidth + this->x),
                              SCALED(y * tileHeight + this->y),
                              0, 
                              0, 
                              SCALED(tileWidth), 
                              SCALED(tileHeight));
    }
    else {
        image->drawSubRect(SCALED(x * tileWidth + this->x), 
                           SCALED(y * tileHeight + this->y),
                           0,
                           SCALED(tileHeight * mapTile.frame),
                           SCALED(tileWidth),
                           SCALED(tileHeight));
    }

    // draw the focus around the tile if it has the focus
    if (focus)
        drawFocus(x, y);
}

void TileView::drawTile(vector<MapTile> &tiles, bool focus, int x, int y) {
	ASSERT(x < columns, "x value of %d out of range", x);
	ASSERT(y < rows, "y value of %d out of range", y);

	animated->fillRect(0,0,SCALED(tileWidth),SCALED(tileHeight),0,0,0, 255);
	animated->drawSubRect(SCALED(x * tileWidth + this->x),
						  SCALED(y * tileHeight + this->y),
						  0,
						  0,
						  SCALED(tileWidth),
						  SCALED(tileHeight));

	//int layer = 0;

	for (vector<MapTile>::reverse_iterator t = tiles.rbegin();
			t != tiles.rend();
			++t)
	{
		MapTile& frontTile = *t;
		Tile *frontTileType = tileset->get(frontTile.id);
		Image *image = frontTileType->getImage();

		//animated->alphaOff();


		// draw the tile to the screen
		if (frontTileType->getAnim()) {
			// First, create our animated version of the tile
			frontTileType->getAnim()->draw(animated, frontTileType, frontTile, DIR_NONE);
		}
		else {
			image->drawSubRectOn(animated,
								0, 0,
								0, SCALED(tileHeight * frontTile.frame),
								SCALED(tileWidth),  SCALED(tileHeight));
		}

		// Then draw it to the screen
		animated->drawSubRect(SCALED(x * tileWidth + this->x),
							  SCALED(y * tileHeight + this->y),
							  0,
							  0,
							  SCALED(tileWidth),
							  SCALED(tileHeight));
	}


	// draw the focus around the tile if it has the focus
	if (focus)
        drawFocus(x, y);
}

/**
 * Draw a focus rectangle around the tile
 */
void TileView::drawFocus(int x, int y) {
    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

    /*
     * draw the focus rectangle around the tile
     */
    if ((screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
        /* left edge */
        screen->fillRect(SCALED(x * tileWidth + this->x),
                         SCALED(y * tileHeight + this->y),
                         SCALED(2),
                         SCALED(tileHeight),
                         0xff, 0xff, 0xff);

        /* top edge */
        screen->fillRect(SCALED(x * tileWidth + this->x),
                         SCALED(y * tileHeight + this->y),
                         SCALED(tileWidth),
                         SCALED(2),
                         0xff, 0xff, 0xff);

        /* right edge */
        screen->fillRect(SCALED((x + 1) * tileWidth + this->x - 2),
                         SCALED(y * tileHeight + this->y),
                         SCALED(2),
                         SCALED(tileHeight),
                         0xff, 0xff, 0xff);

        /* bottom edge */
        screen->fillRect(SCALED(x * tileWidth + this->x),
                         SCALED((y + 1) * tileHeight + this->y - 2),
                         SCALED(tileWidth),
                         SCALED(2),
                         0xff, 0xff, 0xff);
    }
}

void TileView::setTileset(Tileset *tileset) {
    this->tileset = tileset;
}
