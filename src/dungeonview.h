/*
 * dungeonview.h
 */

#ifndef DUNGEONVIEW_H
#define DUNGEONVIEW_H

#include "context.h"
#include "debug.h"
#include "direction.h"
#include "dungeon.h"
#include "location.h"
#include "savegame.h"
#include "tileset.h"
#include "tileview.h"
#include "types.h"

typedef enum {
    DNGGRAPHIC_NONE,
    DNGGRAPHIC_WALL,
    DNGGRAPHIC_LADDERUP,
    DNGGRAPHIC_LADDERDOWN,
    DNGGRAPHIC_LADDERUPDOWN,
    DNGGRAPHIC_DOOR,
    DNGGRAPHIC_DNGTILE,
    DNGGRAPHIC_BASETILE
} DungeonGraphicType;

std::vector<MapTile> dungeonViewGetTiles(int fwd, int side);
DungeonGraphicType dungeonViewTilesToGraphic(const std::vector<MapTile> &tiles);

class DungeonView : public TileView {
public:
    DungeonView(int x, int y, int columns, int rows);

    int graphicIndex(int xoffset, int distance, Direction orientation, DungeonGraphicType type);
    void drawTile(const Tile *tile, int x_offset, int distance, Direction orientation);
    void drawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type);

    void display(Context * c, TileView *view);
    DungeonGraphicType tilesToGraphic(const std::vector<MapTile> &tiles);

    bool toggle3DDungeonView() {
        return screen3dDungeonViewEnabled = ! screen3dDungeonViewEnabled;
    }

    std::vector<MapTile> getTiles(int fwd, int side);
    MapTile black;
    MapTile avatar;
    TileId corridor;
    TileId up_ladder;
    TileId down_ladder;
    TileId updown_ladder;

private:
    bool screen3dDungeonViewEnabled;

    void drawInDungeon(const Tile *tile, int x_offset, int distance, Direction orientation, bool tiled);
};

#endif /* DUNGEONVIEW_H */
