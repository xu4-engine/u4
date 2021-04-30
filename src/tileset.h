/*
 * tileset.h
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include <vector>
#include "types.h"

using std::string;

class Tile;

/**
 * Tileset class
 */
class Tileset {
public:
    typedef std::map<string, const Tile*> TileStrMap;

    static void loadImages();
    static void unloadImages();
    static const Tile* findTileByName(const string &name);
    static const Tile* findTileById(TileId id);

    Tileset() : totalFrames(0) {}
    ~Tileset();

    const Tile* get(TileId id) const;
    const Tile* getByName(const string &name) const;

    string getImageName() const { return imageName; }
    unsigned int numTiles() const { return tiles.size(); }
    unsigned int numFrames() const { return totalFrames; }

//private:
    std::vector<Tile*> tiles;
    TileStrMap nameMap;
    unsigned int totalFrames;
    string imageName;
};

#endif
