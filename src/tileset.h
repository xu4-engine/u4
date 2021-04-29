/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include "types.h"

using std::string;

class Tile;

/**
 * Tileset class
 */
class Tileset {
public:
    typedef std::map<TileId, Tile*> TileIdMap;
    typedef std::map<string, const Tile*> TileStrMap;

    static void loadImages();
    static void unloadImages();
    static const Tileset* get(const string &name);

    static const Tile* findTileByName(const string &name);
    static const Tile* findTileById(TileId id);

    Tileset() : totalFrames(0) {}
    ~Tileset();

    const Tile* get(TileId id) const;
    const Tile* getByName(const string &name) const;
    string getImageName() const;
    unsigned int numTiles() const;
    unsigned int numFrames() const;

//private:
    TileIdMap tiles;
    TileStrMap nameMap;
    unsigned int totalFrames;
    string imageName;
};

#endif
