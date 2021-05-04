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
    typedef std::map<Symbol, const Tile*> TileNameMap;

    static void loadImages();
    static void unloadImages();
    static const Tile* findTileByName(Symbol name);
    static const Tile* findTileById(TileId id);

    Tileset() : totalFrames(0) {}
    ~Tileset();

    const Tile* get(TileId id) const;
    const Tile* getByName(Symbol name) const;

    string getImageName() const { return imageName; }
    unsigned int numTiles() const { return tiles.size(); }
    unsigned int numFrames() const { return totalFrames; }

//private:
    std::vector<Tile*> tiles;
    TileNameMap nameMap;
    unsigned int totalFrames;
    string imageName;
};

#endif
