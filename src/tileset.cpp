/*
 * tileset.cpp
 */

#include "tileset.h"

#include "error.h"
#include "config.h"
#include "imagemgr.h"
#include "settings.h"
#include "tile.h"
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
        std::vector<Tile*>::iterator it;
        foreach (it, ts->tiles) {
            Tile* tile = *it;
            tile->scale = SCALED_BASE;

            j = info->subImageIndex.find(tile->imageName);
            if (j != info->subImageIndex.end()) {
                const SubImage* subimage = info->subImages + j->second;
                tile->frames = subimage->celCount;

                // Set visual to subimage index.
                tile->vid = subimage - info->subImages;
            } else {
                tile->vid = VID_UNSET;
                errorWarning("No subimage found for tile '%s'", tile->nameStr());
            }
            tile->loadImage();
            //printf("KR tile %d %s vid %d frames %d\n",
            //       tile->id, tile->nameStr(), tile->vid, tile->frames);
        }
#else
        std::vector<Tile*>::iterator it;
        foreach (it, ts->tiles)
            (*it)->loadImage();
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
        std::vector<Tile*>::iterator it;
        foreach (it, ts->tiles)
            (*it)->deleteImage();
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

Tileset::~Tileset() {
    /* free all the tiles */
    std::vector<Tile*>::iterator it;
    foreach (it, tiles)
        delete *it;
}

/**
 * Returns the tile with the given id in the tileset
 */
const Tile* Tileset::get(TileId id) const {
    if (id < tiles.size())
        return tiles[id];
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
