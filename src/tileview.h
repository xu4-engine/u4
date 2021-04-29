/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <vector>

#include "view.h"

class Tile;
class Tileset;
class MapTile;

/**
 * A view of a grid of tiles.  Used to draw Maps.
 * @todo
 * <ul>
 *      <li>use for gem view</li>
 *      <li>intialize from a Layout?</li>
 * </ul>
 */
class TileView : public View {
public:
    TileView(int x, int y, int columns, int rows);
    TileView(int x, int y, int columns, int rows, const string &tileset);
    virtual ~TileView();

    void reinit();
    void drawTile(MapTile &mapTile, bool focus, int x, int y);
    void drawTile(std::vector<MapTile> &tiles, bool focus, int x, int y);
    void drawFocus(int x, int y);
    void loadTile(MapTile &mapTile);
    void setTileset(const Tileset *tileset);

protected:
    int columns, rows;
    int tileWidth, tileHeight;
    const Tileset *tileset;
    Image *animated;            /**< a scratchpad image for drawing animations */
};

#endif /* TILEVIEW_H */
