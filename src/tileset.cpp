/*
 * $Id$
 */

#include <vector>

#include "tileset.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tilemap.h"
#include "xu4.h"

using std::vector;

/**
 * Tileset Class Implementation
 */

/* static member variables */
Tileset::TilesetMap Tileset::tilesets;

/**
 * Loads all tilesets using the filename
 * indicated by 'filename' as a definition
 */
void Tileset::loadAll() {
    Debug dbg("debug/tileset.txt", "Tileset");
    vector<ConfigElement> conf;

    TRACE(dbg, "Unloading all tilesets");
    unloadAll();

    // get the config element for all tilesets
    TRACE_LOCAL(dbg, "Loading tilesets info from config");
    conf = xu4.config->getElement("tilesets").getChildren();

    // load all of the tilesets
    for (std::vector<ConfigElement>::iterator i = conf.begin(); i != conf.end(); i++) {
        if (i->getName() == "tileset") {

            Tileset *tileset = new Tileset;
            tileset->load(*i);

            tilesets[tileset->name] = tileset;
        }
    }

    // load tile maps, including translations from index to id
    TRACE_LOCAL(dbg, "Loading tilemaps");
    TileMap::loadAll();

    TRACE(dbg, "Successfully Loaded Tilesets");
}

/**
 * Delete all tilesets
 */
void Tileset::unloadAll() {
    TilesetMap::iterator i;

    // unload all tilemaps
    TileMap::unloadAll();

    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        i->second->unload();
        delete i->second;
    }
    tilesets.clear();

    Tile::resetNextId();
}

/**
 * Delete all tileset images
 */
void Tileset::unloadAllImages() {
    TilesetMap::iterator i;

    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        i->second->unloadImages();
    }

    Tile::resetNextId();
}


/**
 * Returns the tileset with the given name, if it exists
 */
Tileset* Tileset::get(const string &name) {
    if (tilesets.find(name) != tilesets.end())
        return tilesets[name];
    else return NULL;
}

/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
Tile* Tileset::findTileByName(const string &name) {
    TilesetMap::iterator i;
    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        Tile *t = i->second->getByName(name);
        if (t)
            return t;
    }

    return NULL;
}

Tile* Tileset::findTileById(TileId id) {
    TilesetMap::iterator i;
    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        Tile *t = i->second->get(id);
        if (t)
            return t;
    }

    return NULL;
}

/**
 * Loads a tileset.
 */
void Tileset::load(const ConfigElement &tilesetConf) {
    Debug dbg("debug/tileset.txt", "Tileset", true);

    name = tilesetConf.getString("name");
    if (tilesetConf.exists("imageName"))
        imageName = tilesetConf.getString("imageName");
    if (tilesetConf.exists("extends"))
        extends = Tileset::get(tilesetConf.getString("extends"));
    else extends = NULL;

    TRACE_LOCAL(dbg, "\tLoading Tiles...");

    int index = 0;
    vector<ConfigElement> children = tilesetConf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() != "tile")
            continue;

        Tile *tile = new Tile(this);
        tile->loadProperties(*i);

        TRACE_LOCAL(dbg, string("\t\tLoaded '") + tile->getName() + "'");

        /* add the tile to our tileset */
        tiles[tile->getId()] = tile;
        nameMap[tile->getName()] = tile;

        index += tile->getFrames();
    }
    totalFrames = index;
}

void Tileset::unloadImages()
{
    Tileset::TileIdMap::iterator i;

    /* free all the image memory and nullify so that reloading can automatically take place lazily */
    for (i = tiles.begin(); i != tiles.end(); i++)
    {
        i->second->deleteImage();
    }
}

/**
 * Unload the current tileset
 */
void Tileset::unload() {
    Tileset::TileIdMap::iterator i;

    /* free all the memory for the tiles */
    for (i = tiles.begin(); i != tiles.end(); i++)
        delete i->second;

    tiles.clear();
    totalFrames = 0;
    imageName.erase();
}

/**
 * Returns the tile with the given id in the tileset
 */
Tile* Tileset::get(TileId id) {
    if (tiles.find(id) != tiles.end())
        return tiles[id];
    else if (extends)
        return extends->get(id);
    return NULL;
}

/**
 * Returns the tile with the given name from the tileset, if it exists
 */
Tile* Tileset::getByName(const string &name) {
    if (nameMap.find(name) != nameMap.end())
        return nameMap[name];
    else if (extends)
        return extends->getByName(name);
    else return NULL;
}

/**
 * Returns the image name for the tileset, if it exists
 */
string Tileset::getImageName() const {
    if (imageName.empty() && extends)
        return extends->getImageName();
    else return imageName;
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
