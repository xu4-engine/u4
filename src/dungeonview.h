/*
 * $Id$
 */

#ifndef DUNGEONVIEW_H
#define DUNGEONVIEW_H

#include "direction.h"
#include "tileview.h"

class DungeonView : public TileView {
public:
    DungeonView(int x, int y, int columns, int rows);
    DungeonView(int x, int y, int columns, int rows, const string &tileset);

    void drawInDungeon(MapTile *mapTile, int distance, Direction orientation, bool large);
};

#endif /* DUNGEONVIEW_H */
