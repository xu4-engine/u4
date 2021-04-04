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
 * TileRule struct
 */
struct TileRule {
    Symbol name;
    uint16_t mask;
    uint16_t movementMask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
};

/**
 * Tileset class
 */
class Tileset {
public:
    typedef std::map<string, Tileset*> TilesetMap;
    typedef std::map<TileId, Tile*> TileIdMap;
    typedef std::map<string, Tile*> TileStrMap;

    static void loadAll();
    static void unloadAll();
    static void unloadAllImages();
    static Tileset* get(const string &name);

    static Tile* findTileByName(const string &name);
    static Tile* findTileById(TileId id);

public:
    void load(const ConfigElement &tilesetConf);
    void unload();
    void unloadImages();
    Tile* get(TileId id);
    Tile* getByName(const string &name);
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
