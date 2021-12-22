/*
 * dungeonview.h
 */

#ifndef DUNGEONVIEW_H
#define DUNGEONVIEW_H

#include "tileview.h"

typedef enum {
    DNGGRAPHIC_NONE,
    DNGGRAPHIC_WALL,
    DNGGRAPHIC_LADDERUP,
    DNGGRAPHIC_LADDERDOWN,
    DNGGRAPHIC_LADDERUPDOWN,
    DNGGRAPHIC_DOOR,
    DNGGRAPHIC_DNGTILE,
    DNGGRAPHIC_BASETILE,
    DNGGRAPHIC_TRAP
} DungeonGraphicType;

class Context;
class Dungeon;
class ImageInfo;
class SubImage;

class DungeonView : public TileView {
public:
    DungeonView(int x, int y, int columns, int rows);

    void cacheGraphicData();
    void display(Context * c, TileView *view);
    void detectTraps();

    bool toggle3DDungeonView() {
        return screen3dDungeonViewEnabled = ! screen3dDungeonViewEnabled;
    }

private:
    void drawInDungeon(const Tile *tile, int x_offset, int distance, Direction orientation);
    int graphicIndex(const Coords& loc, int xoffset, int distance,
                     Direction orientation, DungeonGraphicType type);
    DungeonGraphicType tilesToGraphic(const Dungeon*,
                                      const std::vector<MapTile> &tiles);
    void drawWall(int graphic);

    struct GraphicData {
        const ImageInfo* info;
        const SubImage* sub;
    };

    MapTile black;
    MapTile avatar;
    TileId corridor;
    TileId up_ladder;
    TileId down_ladder;
    TileId updown_ladder;
    int      spotTrapRange;
    uint32_t spotTrapTime;
    bool screen3dDungeonViewEnabled;
    GraphicData graphic[84];
};

#endif /* DUNGEONVIEW_H */
