/*
 * dungeonview.cpp
 */

#include <assert.h>
#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "dungeonview.h"
#include "error.h"
#include "imagemgr.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"
#include "utils.h"
#include "xu4.h"


DungeonView::DungeonView(int x, int y, int columns, int rows) : TileView(x, y, rows, columns)
, screen3dDungeonViewEnabled(true)
{
    spotTrapRange = -1;

    black  = tileset->getByName(Tile::sym.black)->getId();
    avatar = tileset->getByName(Tile::sym.avatar)->getId();

    corridor      = tileset->getByName(Tile::sym.dungeonFloor)->getId();
    up_ladder     = tileset->getByName(SYM_UP_LADDER)->getId();
    down_ladder   = tileset->getByName(SYM_DOWN_LADDER)->getId();
    updown_ladder = tileset->getByName(SYM_UP_DOWN_LADDER)->getId();

    cacheGraphicData();
}

/*
 * Sets coords relative to party and fills tiles from that location.
 */
static void dungeonGetTiles(Coords& coords, std::vector<MapTile>& tiles,
                            int fwd, int side) {
    coords = c->location->coords;

    switch (c->saveGame->orientation) {
    case DIR_WEST:
        coords.x -= fwd;
        coords.y -= side;
        break;

    case DIR_NORTH:
        coords.x += side;
        coords.y -= fwd;
        break;

    case DIR_EAST:
        coords.x += fwd;
        coords.y += side;
        break;

    case DIR_SOUTH:
        coords.x -= side;
        coords.y += fwd;
        break;

    case DIR_ADVANCE:
    case DIR_RETREAT:
    default:
        ASSERT(0, "Invalid dungeon orientation");
    }

    // Wrap the coordinates if necessary
    map_wrap(coords, c->location->map);

    bool focus;
    tiles.clear();
    c->location->getTilesAt(tiles, coords, focus);
}

void DungeonView::display(Context * c, TileView *view)
{
    static const int8_t wallSides[3] = { -1, 1, 0 };
    Dungeon* dungeon = dynamic_cast<Dungeon *>(c->location->map);
    vector<MapTile> tiles;
    Coords drawLoc;
    int x, y;

    /* 1st-person perspective */
    if (screen3dDungeonViewEnabled) {
        //Note: This shouldn't go above 4, unless we check opaque tiles each step of the way.
        const int farthest_non_wall_tile_visibility = 4;

        screenEraseMapArea();
        if (c->party->getTorchDuration() > 0) {
            vector<MapTile> distant_tiles;

            for (y = 3; y >= 0; y--) {
                DungeonGraphicType type;
                Direction dir = (Direction) c->saveGame->orientation;

                // Draw walls player can see.
                Image::enableBlend(1);
                for (x = 0; x < 3; ++x) {
                    dungeonGetTiles(drawLoc, tiles, y, wallSides[x]);
                    type = tilesToGraphic(dungeon, tiles);
                    drawWall(graphicIndex(drawLoc, wallSides[x], y, dir, type));
                }
                Image::enableBlend(0);

                //This only checks that the tile at y==3 is opaque
                if (y == 3 && !tiles.front().getTileType()->isOpaque())
                {
                    for (int y_obj = farthest_non_wall_tile_visibility; y_obj > y; y_obj--)
                    {
                    dungeonGetTiles(drawLoc, distant_tiles, y_obj, 0);
                    DungeonGraphicType distant_type =
                        tilesToGraphic(dungeon, distant_tiles);

                    if ((distant_type == DNGGRAPHIC_DNGTILE) ||
                        (distant_type == DNGGRAPHIC_BASETILE))
                        drawInDungeon(distant_tiles.front(), 0, y_obj, dir);
                    }
                }
                if ((type == DNGGRAPHIC_DNGTILE) ||
                    (type == DNGGRAPHIC_BASETILE))
                    drawInDungeon(tiles.front(), 0, y, dir);
            }
        }
    }

    /* 3rd-person perspective */
    else {
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                dungeonGetTiles(drawLoc, tiles,
                                (VIEWPORT_H / 2) - y, x - (VIEWPORT_W / 2));

                /* Only show blackness if there is no light */
                if (c->party->getTorchDuration() <= 0)
                    view->drawTile(black, x, y);
                else if (x == VIEWPORT_W/2 && y == VIEWPORT_H/2)
                    view->drawTile(avatar, x, y);
                else
                    view->drawTile(tiles, x, y);
            }
        }
    }
}

void DungeonView::drawInDungeon(const MapTile& mt, int x_offset, int distance, Direction orientation) {
    const static int nscale_vga[] = { 12, 8, 4, 2, 1};
    const static int nscale_ega[] = { 8, 4, 2, 1, 0};

    const int lscale_vga[] = { 22, 18, 10, 4, 1};
    const int lscale_ega[] = { 22, 14, 6, 3, 1};

    const int * lscale;
    const int * nscale;
    int offset_multiplier = 0;
    int offset_adj = 0;
    if (xu4.settings->videoType != "EGA")
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

    //Put tile on animated scratchpad
    const Tile* tile = tileset->get(mt.id);
    if (tile->getAnim())
        tile->getAnim()->draw(animated, tile, mt, orientation);
    else
        tile->getImage()->drawOn(animated, 0, 0);

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    bool tiledWall = tile->isTiledInDungeon();
    const int *dscale = tiledWall ? lscale : nscale;
    Image *scaled;

    if (dscale[distance] == 0)
        return;
    else if (dscale[distance] == 1)
        scaled = scaleDown(animated, 2);
    else
        scaled = scaleUp(animated, dscale[distance] / 2, 1, 0);

    if (tiledWall) {
        int i_x = ((VIEWPORT_W * tileWidth  / 2) + this->x) - (scaled->width() / 2);
        int i_y = ((VIEWPORT_H * tileHeight / 2) + this->y) - (scaled->height() / 2);
        int f_x = i_x + scaled->width();
        int f_y = i_y + scaled->height();
        int d_x = animated->width();
        int d_y = animated->height();

        for (int x = i_x; x < f_x; x+=d_x) {
            for (int y = i_y; y < f_y; y+=d_y)
                animated->drawSubRect(x, y, 0, 0, f_x - x, f_y - y);
        }
    }
    else {
        int y_offset = std::max(0,(dscale[distance] - offset_adj) * offset_multiplier);
        int x = ((VIEWPORT_W * tileWidth / 2) + this->x) - (scaled->width() / 2);
        int y = ((VIEWPORT_H * tileHeight / 2) + this->y + y_offset) - (scaled->height() / 8);

        Image::enableBlend(1);
        scaled->draw(x, y);
        Image::enableBlend(0);
    }

    delete scaled;
}

/*
 * Begin trap detection for the current view.
 * One trap in range may be shown after a delay.
 */
void DungeonView::detectTraps() {
    spotTrapRange = xu4_random(4);
    if (spotTrapRange < 3)
        spotTrapTime = c->commandTimer + 200 + xu4_random(3000);
    else
        spotTrapRange = -1;
}

int DungeonView::graphicIndex(const Coords& loc, int xoffset, int distance,
                              Direction orientation, DungeonGraphicType type) {
    int index;
    assert(distance < 4);

    if (type == DNGGRAPHIC_LADDERUP && xoffset == 0)
        return 48 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);

    if (type == DNGGRAPHIC_LADDERDOWN && xoffset == 0)
        return 56 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);

    if (type == DNGGRAPHIC_LADDERUPDOWN && xoffset == 0)
        return 64 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);

    if (type == DNGGRAPHIC_TRAP) {
#if 1
        if (xoffset == 0 && spotTrapRange == distance &&
             c->commandTimer >= spotTrapTime)
#else
        if (xoffset == 0)   // For Testing
#endif
        {
            index = static_cast<Dungeon *>(c->location->map)->subTokenAt(loc);
            if (index == TRAP_FALLING_ROCK)
                return 78 + distance;
            if (index == TRAP_PIT)
                return 81 + distance;
        }
        return -1;
    }

    /* FIXME */
    if (type != DNGGRAPHIC_WALL && type != DNGGRAPHIC_DOOR)
        return -1;

    index = 0;
    if (type == DNGGRAPHIC_DOOR)
        index += 24;

    index += (xoffset + 1) * 2;
    index += distance * 6;

    if (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH))
        index++;

    return index;
}

DungeonGraphicType DungeonView::tilesToGraphic(const Dungeon* dungeon,
                                        const std::vector<MapTile> &tiles) {
    MapTile tile = tiles.front();

    /*
     * check if the dungeon tile has an annotation or object on top
     * (always displayed as a tile, unless a ladder)
     */
    if (tiles.size() > 1) {
        if (tile.id == up_ladder)
            return DNGGRAPHIC_LADDERUP;
        else if (tile.id == down_ladder)
            return DNGGRAPHIC_LADDERDOWN;
        else if (tile.id == updown_ladder)
            return DNGGRAPHIC_LADDERUPDOWN;
        else if (tile.id == corridor)
            return DNGGRAPHIC_NONE;
        else
            return DNGGRAPHIC_BASETILE;
    }

    /*
     * if not an annotation or object, then the tile is a dungeon
     * token
     */
    DungeonToken token = dungeon->tokenForTile(tile.id);
    switch (token) {
    case DUNGEON_TRAP:
        return DNGGRAPHIC_TRAP;
    case DUNGEON_CORRIDOR:
        return DNGGRAPHIC_NONE;
    case DUNGEON_WALL:
    case DUNGEON_SECRET_DOOR:
        return DNGGRAPHIC_WALL;
    case DUNGEON_ROOM:
    case DUNGEON_DOOR:
        return DNGGRAPHIC_DOOR;
    case DUNGEON_LADDER_UP:
        return DNGGRAPHIC_LADDERUP;
    case DUNGEON_LADDER_DOWN:
        return DNGGRAPHIC_LADDERDOWN;
    case DUNGEON_LADDER_UPDOWN:
        return DNGGRAPHIC_LADDERUPDOWN;

    default:
        return DNGGRAPHIC_DNGTILE;
    }
}

#define GRAPHIC_COUNT   84

const struct {
    const char* imageName;
    uint8_t ega_x2, ega_y2;
    uint8_t vga_x2, vga_y2;
    uint8_t subimage2;
} dngGraphicInfo[GRAPHIC_COUNT] = {
    { "dung0_lft_ew", 0,0,0,0,0 },
    { "dung0_lft_ns", 0,0,0,0,0 },
    { "dung0_mid_ew", 0,0,0,0,0 },
    { "dung0_mid_ns", 0,0,0,0,0 },
    { "dung0_rgt_ew", 0,0,0,0,0 },
    { "dung0_rgt_ns", 0,0,0,0,0 },
        // 6
    { "dung1_lft_ew", 0, 32, 0, 8, 72 },        // + "dung1_xxx_ew"
    { "dung1_lft_ns", 0, 32, 0, 8, 73 },        // + "dung1_xxx_ns"
    { "dung1_mid_ew", 0,0,0,0,0 },
    { "dung1_mid_ns", 0,0,0,0,0 },
    { "dung1_rgt_ew", 144, 32, 160, 8, 72 },    // + "dung1_xxx_ew"
    { "dung1_rgt_ns", 144, 32, 160, 8, 73 },    // + "dung1_xxx_ns"
        // 12
    { "dung2_lft_ew", 0, 64, 0, 48, 74 },       // + "dung2_xxx_ew"
    { "dung2_lft_ns", 0, 64, 0, 48, 75 },       // + "dung2_xxx_ns"
    { "dung2_mid_ew", 0,0,0,0,0 },
    { "dung2_mid_ns", 0,0,0,0,0 },
    { "dung2_rgt_ew", 112, 64, 128, 48, 74 },   // + "dung2_xxx_ew"
    { "dung2_rgt_ns", 112, 64, 128, 48, 75 },   // + "dung2_xxx_ns"
        // 18
    { "dung3_lft_ew", 0, 80, 48, 72, 76 },      // + "dung3_xxx_ew"
    { "dung3_lft_ns", 0, 80, 48, 72, 77 },      // + "dung3_xxx_ns"
    { "dung3_mid_ew", 0,0,0,0,0 },
    { "dung3_mid_ns", 0,0,0,0,0 },
    { "dung3_rgt_ew", 96, 80, 104, 72, 76 },    // + "dung3_xxx_ew"
    { "dung3_rgt_ns", 96, 80, 104, 72, 77 },    // + "dung3_xxx_ns"
        // 24
    { "dung0_lft_ew_door", 0,0,0,0,0 },
    { "dung0_lft_ns_door", 0,0,0,0,0 },
    { "dung0_mid_ew_door", 0,0,0,0,0 },
    { "dung0_mid_ns_door", 0,0,0,0,0 },
    { "dung0_rgt_ew_door", 0,0,0,0,0 },
    { "dung0_rgt_ns_door", 0,0,0,0,0 },
        // 30
    { "dung1_lft_ew_door", 0, 32, 0, 8, 72 },       // + "dung1_xxx_ew"
    { "dung1_lft_ns_door", 0, 32, 0, 8, 73 },       // + "dung1_xxx_ns"
    { "dung1_mid_ew_door", 0,0,0,0,0 },
    { "dung1_mid_ns_door", 0,0,0,0,0 },
    { "dung1_rgt_ew_door", 144, 32, 160, 8, 72 },   // + "dung1_xxx_ew"
    { "dung1_rgt_ns_door", 144, 32, 160, 8, 73 },   // + "dung1_xxx_ns"
        // 36
    { "dung2_lft_ew_door", 0, 64, 0, 48, 74 },      // + "dung2_xxx_ew"
    { "dung2_lft_ns_door", 0, 64, 0, 48, 75 },      // + "dung2_xxx_ns"
    { "dung2_mid_ew_door", 0,0,0,0,0 },
    { "dung2_mid_ns_door", 0,0,0,0,0 },
    { "dung2_rgt_ew_door", 112, 64, 128, 48, 74 },  // + "dung2_xxx_ew"
    { "dung2_rgt_ns_door", 112, 64, 128, 48, 75 },  // + "dung2_xxx_ns"
        // 42
    { "dung3_lft_ew_door", 0, 80, 48, 72, 76 },     // + "dung3_xxx_ew"
    { "dung3_lft_ns_door", 0, 80, 48, 72, 77 },     // + "dung3_xxx_ns"
    { "dung3_mid_ew_door", 0,0,0,0,0 },
    { "dung3_mid_ns_door", 0,0,0,0,0 },
    { "dung3_rgt_ew_door", 96, 80, 104, 72, 76 },   // + "dung3_xxx_ew"
    { "dung3_rgt_ns_door", 96, 80, 104, 72, 77 },   // + "dung3_xxx_ns"
        // 48
    { "dung0_ladderup",      0,0,0,0,0 },
    { "dung0_ladderup_side", 0,0,0,0,0 },
    { "dung1_ladderup",      0,0,0,0,0 },
    { "dung1_ladderup_side", 0,0,0,0,0 },
    { "dung2_ladderup",      0,0,0,0,0 },
    { "dung2_ladderup_side", 0,0,0,0,0 },
    { "dung3_ladderup",      0,0,0,0,0 },
    { "dung3_ladderup_side", 0,0,0,0,0 },
        // 56
    { "dung0_ladderdown",      0,0,0,0,0 },
    { "dung0_ladderdown_side", 0,0,0,0,0 },
    { "dung1_ladderdown",      0,0,0,0,0 },
    { "dung1_ladderdown_side", 0,0,0,0,0 },
    { "dung2_ladderdown",      0,0,0,0,0 },
    { "dung2_ladderdown_side", 0,0,0,0,0 },
    { "dung3_ladderdown",      0,0,0,0,0 },
    { "dung3_ladderdown_side", 0,0,0,0,0 },
        // 64
    { "dung0_ladderupdown",      0,0,0,0,0 },
    { "dung0_ladderupdown_side", 0,0,0,0,0 },
    { "dung1_ladderupdown",      0,0,0,0,0 },
    { "dung1_ladderupdown_side", 0,0,0,0,0 },
    { "dung2_ladderupdown",      0,0,0,0,0 },
    { "dung2_ladderupdown_side", 0,0,0,0,0 },
    { "dung3_ladderupdown",      0,0,0,0,0 },
    { "dung3_ladderupdown_side", 0,0,0,0,0 },
        // 72
    { "dung1_xxx_ew", 0,0,0,0,0 },
    { "dung1_xxx_ns", 0,0,0,0,0 },
    { "dung2_xxx_ew", 0,0,0,0,0 },
    { "dung2_xxx_ns", 0,0,0,0,0 },
    { "dung3_xxx_ew", 0,0,0,0,0 },
    { "dung3_xxx_ns", 0,0,0,0,0 },
        // 78
    { "dung0_hole",   0,0,0,0,0 },
    { "dung1_hole",   0,0,0,0,0 },
    { "dung2_hole",   0,0,0,0,0 },
    { "dung0_pit",    0,0,0,0,0 },
    { "dung1_pit",    0,0,0,0,0 },
    { "dung2_pit",    0,0,0,0,0 }
};

/*
 * Cache wall graphic pointers at setup to avoid lookup by name and image
 * loading during drawWall().
 */
void DungeonView::cacheGraphicData() {
    Symbol name;
    int i;

    for (i = 0; i < GRAPHIC_COUNT; ++i) {
        name = xu4.config->intern(dngGraphicInfo[i].imageName);
        graphic[i].info = xu4.imageMgr->imageInfo(name, &graphic[i].sub);
    }
}

static void drawGraphic(const ImageInfo* info, const SubImage* subimage,
                        int x, int y) {
    x += BORDER_WIDTH;
    y += BORDER_HEIGHT;

    if (subimage) {
        info->image->drawSubRect(x, y, subimage->x, subimage->y,
                                 subimage->width, subimage->height);
    } else
        info->image->draw(x, y);
}

void DungeonView::drawWall(int index) {
    const SubImage* subimage;
    int x, y;
    int i2;

    if (index < 0)
        return;
    if (! graphic[index].info)
        return;

    // TODO: Make all graphics subimages of a single atlas image. This cannot
    // be done until the screen render position of walls is separated from
    // their subimage position.

    subimage = graphic[index].sub;
    if (subimage) {
        x = subimage->x;
        y = subimage->y;
    } else {
        x = y = 0;
    }
    drawGraphic(graphic[index].info, subimage, x, y);

    // FIXME: subimage2 is a horrible hack, needs to be cleaned up
    i2 = dngGraphicInfo[index].subimage2;
    if (i2) {
        if (xu4.settings->videoType == "EGA") {
            x = dngGraphicInfo[index].ega_x2;
            y = dngGraphicInfo[index].ega_y2;
        } else {
            x = dngGraphicInfo[index].vga_x2;
            y = dngGraphicInfo[index].vga_y2;
        }
        drawGraphic(graphic[i2].info, graphic[i2].sub, x, y);
    }
}

