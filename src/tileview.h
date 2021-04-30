/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <vector>

#include "view.h"

class Tileset;

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
    virtual ~TileView();

    void reinit();
    void drawTile(const MapTile &mapTile, bool focus, int x, int y);
    void drawTile(std::vector<MapTile> &tiles, bool focus, int x, int y);
    void drawFocus(int x, int y);
    void loadTile(const MapTile &mapTile);

protected:
    int columns, rows;
    int tileWidth, tileHeight;
    const Tileset *tileset;
    Image *animated;        /**< a scratchpad image for drawing animations */
};

#endif /* TILEVIEW_H */
