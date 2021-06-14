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

        int offset = screenState()->currentCycle * 4 / SCR_CYCLE_PER_SECOND * tile->getScale();
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

//--------------------------------------

TileAnim::~TileAnim()
{
    std::vector<TileAnimTransform *>::iterator ti;
    foreach (ti, transforms)
        delete *ti;
}

static bool drawsTile(const TileAnimTransform* tf)
{
    return (tf->animType == ATYPE_SCROLL || tf->animType == ATYPE_FRAME);
}

void TileAnim::draw(Image *dest, const Tile *tile, const MapTile &mapTile, Direction dir)
{
    if (mapTile.freezeAnimation || (random && xu4_random(100) > random)) {
        // Nothing to do; draw the tile and return!
        tile->getImage()->drawSubRectOn(dest, 0, 0, 0,
                mapTile.frame * tile->getHeight(),
                tile->getWidth(), tile->getHeight());
        return;
    }

    bool drawn = false;
    std::vector<TileAnimTransform *>::const_iterator it;
    foreach (it, transforms) {
        TileAnimTransform* trans = *it;

        if (trans->context == ACON_FRAME) {
            if (mapTile.frame != trans->contextSelect)
                continue;
        } else if (trans->context == ACON_DIR) {
            if (dir != trans->contextSelect)
                continue;
        }

        if (! trans->random || xu4_random(100) < trans->random) {
            if (! drawsTile(trans) && ! drawn) {
                tile->getImage()->drawSubRectOn(dest, 0, 0, 0,
                        mapTile.frame * tile->getHeight(),
                        tile->getWidth(), tile->getHeight());
            }
            trans->draw(dest, tile, mapTile);
            drawn = true;
        }
    }
}

//--------------------------------------

TileAnimSet::~TileAnimSet()
{
    TileAnimMap::iterator it;
    foreach (it, tileanims)
        delete it->second;
}

/**
 * Returns the tile animation with the given name from the current set
 */
TileAnim* TileAnimSet::getByName(Symbol name) const
{
    TileAnimMap::const_iterator i = tileanims.find(name);
    if (i == tileanims.end())
        return NULL;
    return i->second;
}
