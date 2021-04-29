/*
 * $Id$
 */

#include "tileset.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "xu4.h"

/**
 * Loads all tileset images.
 */
void Tileset::loadImages() {
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
        TileIdMap::iterator it;
        for (it = ts->tiles.begin(); it != ts->tiles.end(); ++it)
            it->second->loadImage();
    }
}

/**
 * Delete all tileset images.
 */
void Tileset::unloadImages() {
    Tileset* ts = (Tileset*) xu4.config->tileset();
    if (ts) {
        TileIdMap::iterator it;
        for (it = ts->tiles.begin(); it != ts->tiles.end(); ++it)
            it->second->deleteImage();
    }
}

/**
 * Deprecated!
 * Returns the tileset with the given name, if it exists
 */
const Tileset* Tileset::get(const string &name) {
    return xu4.config->tileset();
}

/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
const Tile* Tileset::findTileByName(const string &name) {
    return xu4.config->tileset()->getByName(name);
}

const Tile* Tileset::findTileById(TileId id) {
    return xu4.config->tileset()->get(id);
}

Tileset::~Tileset() {
    /* free all the tiles */
    TileIdMap::iterator it;
    foreach (it, tiles)
        delete it->second;
}

/**
 * Returns the tile with the given id in the tileset
 */
const Tile* Tileset::get(TileId id) const {
    TileIdMap::const_iterator it = tiles.find(id);
    if (it != tiles.end())
        return it->second;
    return NULL;
}

/**
 * Returns the tile with the given name from the tileset, if it exists
 */
const Tile* Tileset::getByName(const string &name) const {
    TileStrMap::const_iterator it = nameMap.find(name);
    if (it != nameMap.end())
        return it->second;
    return NULL;
}

/**
 * Returns the image name for the tileset, if it exists
 */
string Tileset::getImageName() const {
    return imageName;
}

/**
 * Returns the number of tiles in the tileset
 */
unsigned int Tileset::numTiles() const {
    return tiles.size();
}

/**
 * Returns the total number of frames in the tileset
 */
unsigned int Tileset::numFrames() const {
    return totalFrames;
}
