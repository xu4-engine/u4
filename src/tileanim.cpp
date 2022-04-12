/*
 * tileanim.cpp
 */

#include "config.h"
#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "u4.h"
#include "utils.h"
#include "tile.h"
#include "xu4.h"


void TileAnimTransform::draw(Image* dest, const Tile* tile,
                             const MapTile& mapTile)
{
    switch(animType) {
    case ATYPE_INVERT:
    {
        int x = var.invert.x;
        int y = var.invert.y;

        tile->getImage()->drawSubRectInvertedOn(dest, x, y,
                x, (tile->getHeight() * mapTile.frame) + y,
                var.invert.w, var.invert.h);
    }
        break;

    case ATYPE_SCROLL:
    {
        if (var.scroll.increment == 0)
            var.scroll.increment = 1;

        int offset = screenState()->currentCycle * 4 / SCR_CYCLE_PER_SECOND;
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
    {
        int frame;
        if (xu4.stage == StagePlay) {
            // flourishAnim drives Object frame animation.
            frame = mapTile.frame;
        } else {
            // The Intro map uses this older code which shares a single
            // frame counter for all tiles.

            // Advance the frame by one and draw it!
            if (++var.frame.current >= tile->getFrames())
                var.frame.current = 0;
            frame = var.frame.current;
        }
        tile->getImage()->drawSubRectOn(dest, 0, 0,
                0, frame * tile->getHeight(),
                tile->getWidth(), tile->getHeight());
    }
        break;
#if 0
    case ATYPE_PIXEL:
    {
        RGBA color = var.pixel.colors[ xu4_random(colors.size()) ];
        dest->fillRect(x, y, 1, 1,
                       color.r, color.g, color.b, color.a);
    }
        break;
#endif
    case ATYPE_PIXEL_COLOR:
    {
        const Image *tileImage = tile->getImage();
        int x = var.pcolor.x;
        int y = var.pcolor.y;
        int w = var.pcolor.w;
        int h = var.pcolor.h;
        RGBA start = var.pcolor.start;
        RGBA end   = var.pcolor.end;
        RGBA diff  = end;

        diff.r -= start.r;
        diff.g -= start.g;
        diff.b -= start.b;
#if 0
        printf( "PC color %d,%d,%d\n", start.r, start.g, start.b );
        printf( "   end   %d,%d,%d\n", end.r, end.g, end.b );
        printf( "   diff  %d,%d,%d\n", diff.r, diff.g, diff.b );
#endif

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
#ifndef USE_BORON
    std::vector<TileAnimTransform *>::iterator ti;
    foreach (ti, transforms)
        delete *ti;
#endif
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
