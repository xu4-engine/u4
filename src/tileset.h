/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include "types.h"

using std::string;

class ConfigElement;
class Tile;

/**
 * Tileset class
 */
class Tileset {
public:
    typedef std::map<string, Tileset*> TilesetMap;
    typedef std::map<TileId, Tile*> TileIdMap;
    typedef std::map<string, const Tile*> TileStrMap;

    static void loadAll();
    static void unloadAll();
    static void loadAllImages();
    static void unloadAllImages();
    static Tileset* get(const string &name);

    static const Tile* findTileByName(const string &name);
    static const Tile* findTileById(TileId id);

    void load(const ConfigElement &tilesetConf);
    void unload();
    void loadImages();
    void unloadImages();
    const Tile* get(TileId id) const;
    const Tile* getByName(const string &name) const;
    string getImageName() const;
    unsigned int numTiles() const;
    unsigned int numFrames() const;

private:
    static TilesetMap tilesets;

    string name;
    TileIdMap tiles;
    unsigned int totalFrames;
    string imageName;
    Tileset* extends;

    TileStrMap nameMap;
};

#endif
