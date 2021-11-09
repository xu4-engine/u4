/*
 * tileset.h
 */

#ifndef TILESET_H
#define TILESET_H

#include <map>
#include "tile.h"

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

    Tileset(int count);
    ~Tileset();

    const Tile* get(TileId id) const;
    const Tile* getByName(Symbol name) const;

    Tile* tiles;
    TileRenderData* render;
    uint32_t tileCount;
    TileNameMap nameMap;
};

#endif
