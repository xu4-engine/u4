/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include "view.h"

class Tileset;
class MapTile;

/**
 * A view of a grid of tiles.  Used to draw Maps.
 */
class TileView : public View {
public:
    TileView(int x, int y, int columns, int rows);
    TileView(int x, int y, int columns, int rows, const string &tileset);
    virtual ~TileView();

    void drawTile(MapTile *mapTile, bool focus, int x, int y);
    void drawFocus(int x, int y);

protected:
    int columns, rows;
    int tileWidth, tileHeight;
    Tileset *tileset;
    Image *animated;            /**< a scratchpad image for drawing animations */
};

#endif /* TILEVIEW_H */
