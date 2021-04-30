/*
 * $Id$
 */

#include "config.h"
#include "debug.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tile.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "u4.h"
#include "error.h"
#include "xu4.h"

using std::vector;

TileView::TileView(int x, int y, int columns, int rows) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    tileWidth  = TILE_WIDTH;
    tileHeight = TILE_HEIGHT;
    tileset = xu4.config->tileset();
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight));
}

TileView::~TileView() {
    delete animated;
}

void TileView::reinit() {
    View::reinit();

    //Scratchpad needs to be re-inited if we rescale...
    if (animated)
    {
        delete animated;
        animated = NULL;
    }
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight));
}

void TileView::loadTile(const MapTile &mapTile)
{
    //This attempts to preload tiles in advance
    const Tile *tile = tileset->get(mapTile.id);
    if (tile)
        tile->getImage();

    //But may fail if the tiles don't exist directly in the expected imagesets
}

/*
 * Draw a tile on the screenImage using the current Image::enableBlend
 * setting.
 */
void TileView::drawTile(const MapTile &mapTile, bool focus, int x, int y) {
    const Tile *tile = tileset->get(mapTile.id);

    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

#if 0
    //Blank scratch pad
    animated->fillRect(0,0,SCALED(tileWidth),SCALED(tileHeight),0,0,0, 255);
    //Draw blackness on the tile.
    animated->drawSubRect(SCALED(x * tileWidth + this->x),
                          SCALED(y * tileHeight + this->y),
                          0,
                          0,
                          SCALED(tileWidth),
                          SCALED(tileHeight));
#endif

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
        const Image *image = tile->getImage();
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
    int layer = 0;

    //animated->fillRect(0,0, SCALED(tileWidth),SCALED(tileHeight), 0,0,0,0);

    for (vector<MapTile>::reverse_iterator t = tiles.rbegin();
            t != tiles.rend();
            ++t, ++layer)
    {
        const MapTile& frontTile = *t;
        const Tile *frontTileType = tileset->get(frontTile.id);

        if (!frontTileType)
        {
            //TODO, this leads to an error. It happens after graphics mode changes.
            return;
        }

        // draw the tile to the screen
        if (frontTileType->getAnim()) {
            // First, create our animated version of the tile
            frontTileType->getAnim()->draw(animated, frontTileType, frontTile, DIR_NONE);
        }
        else {
            const Image *image = frontTileType->getImage();
            if (!image)
                return; //This is a problem //FIXME, error message it.

            image->drawSubRectOn(animated,
                                0, 0,
                                0, SCALED(tileHeight * frontTile.frame),
                                SCALED(tileWidth),  SCALED(tileHeight));
        }

        // Enable blending after the first tile (assuming it's the ground).
        Image::enableBlend(1);
    }

    // Keep blending disabled by default.
    Image::enableBlend(0);

    // Then draw it to the screen
    animated->drawSubRect(SCALED(x * tileWidth + this->x),
                          SCALED(y * tileHeight + this->y),
                          0, 0, SCALED(tileWidth), SCALED(tileHeight));

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
    Image* screen = xu4.screenImage;

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
