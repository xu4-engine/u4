/*
 * $Id$
 */

#ifdef GPU_RENDER
#include <string.h>
#include "event.h"
#include "image32.h"
#include "gpu.h"
#endif

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
    SCALED_VAR
    animated = Image::create(SCALED(tileWidth), SCALED(tileHeight));

#ifdef GPU_RENDER
    effectCount = 0;
    memset(effect, 0, sizeof(effect));      // Sets method to VE_FREE.
#endif
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
    SCALED_VAR
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
void TileView::drawTile(const MapTile &mapTile, int x, int y) {
    const Tile *tile = tileset->get(mapTile.id);
    SCALED_VAR

    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

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
}

void TileView::drawTile(vector<MapTile> &tiles, int x, int y) {
    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);
    int layer = 0;
    SCALED_VAR

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
}

/**
 * Draw a focus rectangle around the tile
 */
void TileView::drawFocus(int x, int y) {
    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);
    Image* screen = xu4.screenImage;
    SCALED_VAR

    /*
     * draw the focus rectangle around the tile
     */
    if ((screenState()->currentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
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

#ifdef GPU_RENDER
/*
 * Return effect id or -1 if effect limit is reached.
 */
int TileView::showEffect(const Coords &coords, TileId tile, AnimId moveAnim) {
    for (int i = 0; i < VE_MAX; ++i) {
        if (effect[i].method == VE_FREE) {
            VisualEffect* it = effect + i;
            it->pos[0] = (float) coords.x;
            it->pos[1] = (float) coords.y;
            it->method = (moveAnim == ANIM_UNUSED) ? VE_SPRITE : VE_SPRITE_MOVE;
            it->vid = tileset->render[tile].vid;
            it->anim = moveAnim;
            if (i >= effectCount)
                effectCount = i + 1;
            return i;
        }
    }
    return -1;
}

void TileView::removeEffect(int id) {
    if (id >= 0) {
        VisualEffect* it = effect + id;
        it->method = VE_FREE;
        int last = effectCount - 1;
        if (id == last) {
            do {
                --last;
            } while (last >= 0 && effect[last].method == VE_FREE);
            effectCount = last + 1;
        }
    }
}

#define VIEW_TILE_SIZE  (2.0f / VIEWPORT_W)

/*
 * \param cx        View center X.
 * \param cy        View center Y.
 * \param uvTable   Texture coordinate min & max indexed by VisualId.
 */
void TileView::updateEffects(float cx, float cy, const float* uvTable) {
    const int TRIS_MAP_FX = 1;
    const float halfTile = VIEW_TILE_SIZE * -0.5f;

    if (effectCount) {
        const Animator* fxAnim = &xu4.eventHandler->fxAnim;
        float* animPos;
        float* attr = gpu_beginTris(xu4.gpu, TRIS_MAP_FX);
        float rect[4];
        int uvIndex;
        VisualEffect* it = effect;
        VisualEffect* end = it + effectCount;

        rect[2] = rect[3] = VIEW_TILE_SIZE;

        for (; it != end; ++it) {
            switch (it->method) {
                case VE_SPRITE_MOVE:
                    animPos = anim_valueF2(fxAnim, it->anim);
                    it->pos[0] = animPos[0];
                    it->pos[1] = animPos[1];
                    // Fall through...

                case VE_SPRITE:
                    // Similar to emitSprite() in screen.cpp.
                    rect[0] = halfTile + (it->pos[0] - cx) * VIEW_TILE_SIZE;
                    rect[1] = halfTile + (cy - it->pos[1]) * VIEW_TILE_SIZE;
                    uvIndex = VID_INDEX(it->vid);
                    attr = gpu_emitQuad(attr, rect, uvTable + uvIndex*4);
                    break;
            }
        }
        gpu_endTris(xu4.gpu, TRIS_MAP_FX, attr);
    } else {
        gpu_clearTris(xu4.gpu, TRIS_MAP_FX);
    }
}
#endif
