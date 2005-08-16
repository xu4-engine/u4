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
    this->tileset = Tileset::get();
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight), false, Image::HARDWARE);
}

TileView::TileView(int x, int y, int columns, int rows, const string &tileset) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->tileWidth = TILE_WIDTH;
    this->tileHeight = TILE_HEIGHT;
    this->tileset = Tileset::get(tileset);
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight), false, Image::HARDWARE);
}

TileView::~TileView() {
    delete animated;
}

void TileView::drawTile(MapTile *mapTile, bool focus, int x, int y) {
    Tile *tile = handleMissingTiles(mapTile);
    Image *image = tile->getImage();

    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

    // draw the tile to the screen
    if (tile->anim) {
        // First, create our animated version of the tile
        tile->anim->draw(animated, tile, mapTile, DIR_NONE);

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
                           SCALED(tileHeight * mapTile->frame),
                           SCALED(tileWidth),
                           SCALED(tileHeight));
    }

    // draw the focus around the tile if it has the focus
    if (focus)
        drawFocus(x, y);
}

void TileView::drawTile(const vector<MapTile *> &tiles, bool focus, int x, int y) {
    Tile *tile = handleMissingTiles(tiles.front());
    Image *image = tile->getImage();

    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

    // draw the tile to the screen
    if (tile->anim) {
        // First, create our animated version of the tile
        tile->anim->draw(animated, tile, tiles.front(), DIR_NONE);

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
                           SCALED(tileHeight * tiles.front()->frame),
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

Tile *TileView::handleMissingTiles(MapTile *mapTile) {
    int id = mapTile->id;
    switch (id) {
        // Handle missing tiles.  We should probably do this
        // in a config file, but putting this now stops a crash.
    case 109: id = 38;  break; // double-ladder as ship wheel
    case 110: id = 58;  break; // trap as lava
    case 112: id = 60;  break; // orb as magic-hit
    case 113: id = 2;   break; // fountain as shallows
    case 114: id = 71;  break; // room as spacer
    case 115: id = 43;  break; // door as door
    case 116: id = 56;  break; // altar as altar
    }
    Tile *tile = tileset->get(id);
    if (!tile)
        tile = tileset->get(4);    // any other failues replaced with grass
    return tile;
}
