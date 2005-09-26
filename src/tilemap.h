/**
 * $Id$
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include <map>
#include <string>
#include "types.h"

class ConfigElement;

using std::string;

/**
 * A tilemap maps the raw bytes in a map file to MapTiles.
 */
class TileMap {
public:
    typedef std::map<string, TileMap *> TileIndexMapMap;
    
    MapTile translate(unsigned int index);
    unsigned int untranslate(MapTile &tile);

    static void loadAll();
    static void unloadAll();
    static TileMap *get(string name);

private:
    static void load(const ConfigElement &tilemapConf);
    static TileIndexMapMap tileMaps;

    std::map<unsigned int, MapTile> tilemap;
};

#endif


