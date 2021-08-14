/*
 * dungeonview.cpp
 */

#include "debug.h"
#include "dungeonview.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"
#include "error.h"
#include "xu4.h"


DungeonView::DungeonView(int x, int y, int columns, int rows) : TileView(x, y, rows, columns)
, screen3dDungeonViewEnabled(true)
{
    black  = tileset->getByName(Tile::sym.black)->getId();
    avatar = tileset->getByName(Tile::sym.avatar)->getId();

    corridor      = tileset->getByName(Tile::sym.dungeonFloor)->getId();
    up_ladder     = tileset->getByName(SYM_UP_LADDER)->getId();
    down_ladder   = tileset->getByName(SYM_DOWN_LADDER)->getId();
    updown_ladder = tileset->getByName(SYM_UP_DOWN_LADDER)->getId();

    cacheGraphicData();
}

void DungeonView::display(Context * c, TileView *view)
{
    static const int8_t wallSides[3] = { -1, 1, 0 };
    vector<MapTile> tiles;
    int x,y;

    /* 1st-person perspective */
    if (screen3dDungeonViewEnabled) {
        //Note: This shouldn't go above 4, unless we check opaque tiles each step of the way.
        const int farthest_non_wall_tile_visibility = 4;

        screenEraseMapArea();
        if (c->party->getTorchDuration() > 0) {
            for (y = 3; y >= 0; y--) {
                DungeonGraphicType type;
                Direction dir = (Direction) c->saveGame->orientation;

                // Draw walls player can see.
                for (x = 0; x < 3; ++x) {
                    tiles = getTiles(y, wallSides[x]);
                    type = tilesToGraphic(tiles);
                    drawWall(wallSides[x], y, dir, type);
                }

                //This only checks that the tile at y==3 is opaque
                if (y == 3 && !tiles.front().getTileType()->isOpaque())
                {
                    for (int y_obj = farthest_non_wall_tile_visibility; y_obj > y; y_obj--)
                    {
                    vector<MapTile> distant_tiles = getTiles(y_obj, 0);
                    DungeonGraphicType distant_type = tilesToGraphic(distant_tiles);

                    if ((distant_type == DNGGRAPHIC_DNGTILE) ||
                        (distant_type == DNGGRAPHIC_BASETILE))
                        drawInDungeon(tileset->get(distant_tiles.front().getId()), 0, y_obj, dir);
                    }
                }
                if ((type == DNGGRAPHIC_DNGTILE) ||
                    (type == DNGGRAPHIC_BASETILE))
                    drawInDungeon(tileset->get(tiles.front().getId()), 0, y, dir);
            }
        }
    }

    /* 3rd-person perspective */
    else {
        for (y = 0; y < VIEWPORT_H; y++) {
            for (x = 0; x < VIEWPORT_W; x++) {
                tiles = getTiles((VIEWPORT_H / 2) - y, x - (VIEWPORT_W / 2));

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

void DungeonView::drawInDungeon(const Tile *tile, int x_offset, int distance, Direction orientation) {
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

#if 0
    //Clear scratchpad and set a background color
    RGBA darkGray(14, 15, 16, 0);
    animated->fill(darkGray);
#endif

    //Put tile on animated scratchpad
    if (tile->getAnim()) {
        MapTile mt = tile->getId();
        tile->getAnim()->draw(animated, tile, mt, orientation);
    }
    else
    {
        tile->getImage()->drawOn(animated, 0, 0);
    }

#if 0
    animated->makeColorTransparent(darkGray);
    //This process involving the background color is only required for drawing in the dungeon.
#endif

    /* scale is based on distance; 1 means half size, 2 regular, 4 means scale by 2x, etc. */
    bool tiledWall = tile->isTiledInDungeon();
    const int *dscale = tiledWall ? lscale : nscale;
    SCALED_VAR
    Image *scaled;

    if (dscale[distance] == 0)
        return;
    else if (dscale[distance] == 1)
        scaled = screenScaleDown(animated, 2);
    else
        scaled = screenScale(animated, dscale[distance] / 2, 1, 0);

    if (tiledWall) {
        int i_x = SCALED((VIEWPORT_W * tileWidth  / 2) + this->x) - (scaled->width() / 2);
        int i_y = SCALED((VIEWPORT_H * tileHeight / 2) + this->y) - (scaled->height() / 2);
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
        int x = SCALED((VIEWPORT_W * tileWidth / 2) + this->x) - (scaled->width() / 2);
        int y = SCALED((VIEWPORT_H * tileHeight / 2) + this->y + y_offset) - (scaled->height() / 8);

        Image::enableBlend(1);
        scaled->draw(x, y);
        Image::enableBlend(0);
    }

    delete scaled;
}

int DungeonView::graphicIndex(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;

    index = 0;

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

    /* FIXME */
    if (type != DNGGRAPHIC_WALL && type != DNGGRAPHIC_DOOR)
        return -1;

    if (type == DNGGRAPHIC_DOOR)
        index += 24;

    index += (xoffset + 1) * 2;

    index += distance * 6;

    if (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH))
        index++;

    return index;
}

std::vector<MapTile> DungeonView::getTiles(int fwd, int side) {
    Coords coords = c->location->coords;

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
    return c->location->tilesAt(coords, focus);
}

DungeonGraphicType DungeonView::tilesToGraphic(const std::vector<MapTile> &tiles) {
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
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    DungeonToken token = dungeon->tokenForTile(tile.id);

    switch (token) {
    case DUNGEON_TRAP:
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

#define GRAPHIC_COUNT   78

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
    { "dung3_xxx_ns", 0,0,0,0,0 }
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
                        int x, int y, int sscale) {
    x = SCALED(BORDER_WIDTH  + x);
    y = SCALED(BORDER_HEIGHT + y);

    if (subimage) {
        int scalep = sscale / info->prescale;
        info->image->drawSubRect(x, y,
                                 subimage->x * scalep,
                                 subimage->y * scalep,
                                 subimage->width * scalep,
                                 subimage->height * scalep);
    } else
        info->image->draw(x, y);
}

void DungeonView::drawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    const SubImage* subimage;
    int index, i2;
    int x, y;
    unsigned int scale = SCALED_BASE;

    index = graphicIndex(xoffset, distance, orientation, type);
    if (index == -1 || distance >= 4)
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
    drawGraphic(graphic[index].info, subimage, x, y, scale);

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
        drawGraphic(graphic[i2].info, graphic[i2].sub, x, y, scale);
    }
}

