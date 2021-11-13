/*
 * tileset.cpp
 */

#include <cstring>
#include "tileset.h"

#include "error.h"
#include "imagemgr.h"
#include "tileanim.h"
#include "xu4.h"

/**
 * Loads all tileset images.
 */
void Tileset::loadImages() {
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
#ifdef GPU_RENDER
        Symbol texture = xu4.config->intern("texture");
        ImageInfo* info = xu4.imageMgr->get(texture);
        if (! info)
            return;

        std::map<Symbol, int>::iterator j;
        Tile* tile = ts->tiles;
        Tile* end  = tile + ts->tileCount;
        TileRenderData* rit = ts->render;
        for (; tile != end; ++rit, ++tile) {
            rit->animType = -1; //ATYPE_NONE;
            rit->animData.scroll = 0;   // Initialize animData to zero.

            j = info->subImageIndex.find(tile->imageName);
            if (j != info->subImageIndex.end()) {
                const SubImage* subimage = info->subImages + j->second;
                tile->w = subimage->width;
                tile->h = subimage->height;
                tile->frames = subimage->celCount;

                // Set visual to subimage index.
                rit->vid = subimage - info->subImages;

                tile->loadImage();      // Set anim.
                if (tile->anim) {
                    const TileAnimTransform* trans = tile->anim->transforms[0];
                    if (trans->animType == ATYPE_SCROLL) {
                        if (trans->var.scroll.vid)
                            rit->animData.scroll = trans->var.scroll.vid;
                        else
                            rit->animData.scroll = rit->vid;
                    } else if (trans->animType == ATYPE_PIXEL_COLOR) {
                        // Assuming this a campfire; store center X.
                        rit->animData.hot[0] = trans->var.pcolor.x +
                                               (trans->var.pcolor.w+1) / 2;
                        rit->animData.hot[1] = tile->w;
                    }
                    rit->animType = trans->animType;
                }
            } else {
                rit->vid = VID_UNSET;
                errorWarning("No subimage found for tile '%s'", tile->nameStr());
            }
            //printf("KR tile %d %s vid %d frames %d\n",
            //       tile->id, tile->nameStr(), rit->vid, tile->frames);
        }
#else
        Tile* it  = ts->tiles;
        Tile* end = it + ts->tileCount;
        for (; it != end; ++it)
            it->loadImage();
#endif
    }
}

/**
 * Delete all tileset images.
 */
void Tileset::unloadImages() {
#ifndef GPU_RENDER
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
        Tile* it  = ts->tiles;
        Tile* end = it + ts->tileCount;
        for (; it != end; ++it)
            it->deleteImage();
    }
#endif
}

/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
const Tile* Tileset::findTileByName(Symbol name) {
    return xu4.config->tileset()->getByName(name);
}

const Tile* Tileset::findTileById(TileId id) {
    return xu4.config->tileset()->get(id);
}

Tileset::Tileset(int count) : tileCount(0) {
    tiles  = new Tile[count];
    render = new TileRenderData[count];
    memset(tiles, 0, sizeof(Tile) * count);
}

Tileset::~Tileset() {
    delete[] tiles;
    delete[] render;
}

/**
 * Returns the tile with the given id in the tileset
 */
const Tile* Tileset::get(TileId id) const {
    if (id < tileCount)
        return tiles + id;
    return NULL;
}

/**
 * Returns the tile with the given name from the tileset, if it exists
 */
const Tile* Tileset::getByName(Symbol name) const {
    TileNameMap::const_iterator it = nameMap.find(name);
    if (it != nameMap.end())
        return it->second;
    return NULL;
}
