/*
 * tileset.cpp
 */

#include "tileset.h"

#include "config.h"
#include "tile.h"
#include "xu4.h"

/**
 * Loads all tileset images.
 */
void Tileset::loadImages() {
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
        std::vector<Tile*>::iterator it;
        foreach (it, ts->tiles)
            (*it)->loadImage();
    }
}

/**
 * Delete all tileset images.
 */
void Tileset::unloadImages() {
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
        std::vector<Tile*>::iterator it;
        foreach (it, ts->tiles)
            (*it)->deleteImage();
    }
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
