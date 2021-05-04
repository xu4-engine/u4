/*
 * tileanim.cpp
 */

#include <vector>

#include "config.h"
#include "direction.h"
#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "u4.h"
#include "utils.h"
#include "tile.h"


static bool drawsTile(const TileAnimTransform* tf)
{
    return (tf->animType == ATYPE_SCROLL || tf->animType == ATYPE_FRAME);
}

void TileAnimTransform::draw(Image* dest, const Tile* tile,
                             const MapTile& mapTile)
{
    switch(animType) {
    case ATYPE_INVERT:
    {
        int scale = tile->getScale();
        int x = var.invert.x * scale;
        int y = var.invert.y * scale;

        tile->getImage()->drawSubRectInvertedOn(dest, x, y,
                x, (tile->getHeight() * mapTile.frame) + y,
                var.invert.w * scale, var.invert.h * scale);
    }
        break;

    case ATYPE_SCROLL:
    {
        if (var.scroll.increment == 0)
            var.scroll.increment = tile->getScale();

        int offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND * tile->getScale();
        if (var.scroll.lastOffset != offset) {
            var.scroll.lastOffset = offset;
            var.scroll.current += var.scroll.increment;
            if (var.scroll.current >= tile->getHeight())
                var.scroll.current = 0;
        }

        tile->getImage()->drawSubRectOn(dest, 0, var.scroll.current,
                0, tile->getHeight() * mapTile.frame,
                tile->getWidth(), tile->getHeight() - var.scroll.current);
        if (var.scroll.current != 0) {
            tile->getImage()->drawSubRectOn(dest, 0, 0,
                    0, (tile->getHeight() * mapTile.frame) + tile->getHeight() - var.scroll.current,
                    tile->getWidth(), var.scroll.current);
        }
    }
        break;

    case ATYPE_FRAME:
        // Advance the frame by one and draw it!
        if (++var.frame.current >= tile->getFrames())
            var.frame.current = 0;
        tile->getImage()->drawSubRectOn(dest, 0, 0,
                0, var.frame.current * tile->getHeight(),
                tile->getWidth(), tile->getHeight());
        break;
#if 0
    case ATYPE_PIXEL:
    {
        RGBA color = var.pixel.colors[ xu4_random(colors.size()) ];
        int scale = tile->getScale();
        dest->fillRect(x * scale, y * scale, scale, scale,
                       color.r, color.g, color.b, color.a);
    }
        break;
#endif
    case ATYPE_PIXEL_COLOR:
    {
        const Image *tileImage = tile->getImage();
        int scale = tile->getScale();
        int x = var.pcolor.x * scale;
        int y = var.pcolor.y * scale;
        int w = var.pcolor.w * scale;
        int h = var.pcolor.h * scale;
        RGBA start = var.pcolor.start;
        RGBA end   = var.pcolor.end;
        RGBA diff  = end;

        diff.r -= start.r;
        diff.g -= start.g;
        diff.b -= start.b;

        for (int j = y; j < y + h; j++) {
            for (int i = x; i < x + w; i++) {
                RGBA pixelAt;
                tileImage->getPixel(i, j + (mapTile.frame * tile->getHeight()), pixelAt);
                if (pixelAt.r >= start.r && pixelAt.r <= end.r &&
                    pixelAt.g >= start.g && pixelAt.g <= end.g &&
                    pixelAt.b >= start.b && pixelAt.b <= end.b) {
                    dest->putPixel(i, j, start.r + xu4_random(diff.r),
                                         start.g + xu4_random(diff.g),
                                         start.b + xu4_random(diff.b),
                                         pixelAt.a);
                }
            }
        }
    }
        break;
    }
}

TileAnimContext::~TileAnimContext() {
    TileAnimTransformList::iterator it;
    foreach (it, transforms)
        delete *it;
}

/**
 * A context which depends on the tile's current frame for animation
 */
TileAnimFrameContext::TileAnimFrameContext(int f) : frame(f) {}

bool TileAnimFrameContext::isInContext(const Tile *t, const MapTile &mapTile, Direction dir) {
    return (mapTile.frame == frame);
}

/**
 * An animation context which changes the animation based on the player's current facing direction
 */
TileAnimPlayerDirContext::TileAnimPlayerDirContext(Direction d) : dir(d) {}
bool TileAnimPlayerDirContext::isInContext(const Tile *t, const MapTile &mapTile, Direction d) {
    return (d == dir);
}

/**
 * ~TileAnimSet
 */
TileAnimSet::~TileAnimSet() {
    TileAnimMap::iterator it;
    foreach (it, tileanims)
        delete it->second;
}

/**
 * Returns the tile animation with the given name from the current set
 */
TileAnim *TileAnimSet::getByName(const std::string &name) {
    TileAnimMap::iterator i = tileanims.find(name);
    if (i == tileanims.end())
        return NULL;
    return i->second;
}

TileAnim::~TileAnim() {
    std::vector<TileAnimTransform *>::iterator ti;
    foreach (ti, transforms)
        delete *ti;

    std::vector<TileAnimContext *>::iterator ci;
    foreach (ci, contexts)
        delete *ci;
}

void TileAnim::draw(Image *dest, const Tile *tile, const MapTile &mapTile, Direction dir) {
    std::vector<TileAnimTransform *>::const_iterator t;
    std::vector<TileAnimContext *>::const_iterator c;
    bool drawn = false;

    /* nothing to do, draw the tile and return! */
    if ((random && xu4_random(100) > random) ||
        (!transforms.size() && !contexts.size()) || mapTile.freezeAnimation) {
        tile->getImage()->drawSubRectOn(dest, 0, 0, 0,
                mapTile.frame * tile->getHeight(),
                tile->getWidth(), tile->getHeight());
        return;
    }

    /**
     * Do global transforms
     */
    for (t = transforms.begin(); t != transforms.end(); t++) {
        TileAnimTransform *transform = *t;

        if (!transform->random || xu4_random(100) < transform->random) {
            if (!drawsTile(transform) && !drawn) {
                tile->getImage()->drawSubRectOn(dest, 0, 0, 0,
                        mapTile.frame * tile->getHeight(),
                        tile->getWidth(), tile->getHeight());
            }
            transform->draw(dest, tile, mapTile);
            drawn = true;
        }
    }

    /**
     * Do contextual transforms
     */
    for (c = contexts.begin(); c != contexts.end(); c++) {
        if ((*c)->isInContext(tile, mapTile, dir)) {
            TileAnimContext::TileAnimTransformList& ctx_transforms = (*c)->transforms;
            for (t = ctx_transforms.begin(); t != ctx_transforms.end(); t++) {
                TileAnimTransform *transform = *t;

                if (!transform->random || xu4_random(100) < transform->random) {
                    if (!drawsTile(transform) && !drawn) {
                        tile->getImage()->drawSubRectOn(dest, 0, 0, 0,
                                mapTile.frame * tile->getHeight(),
                                tile->getWidth(), tile->getHeight());
                    }
                    transform->draw(dest, tile, mapTile);
                    drawn = true;
                }
            }
        }
    }
}
