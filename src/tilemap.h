/**
 * $Id$
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include <map>
#include <string>
#include "types.h"

using std::string;

typedef std::map<unsigned int, string> TileIndexMap;

class TileMap {
public:
    typedef std::map<string, TileIndexMap*> TileIndexMapMap;
    
    static void loadAll(string filename);
    static void unloadAll();
    static void load(string filename);
    static TileIndexMap* get(string name);
    static int size();
    static const string& getName(unsigned int tile, string map = "base");

private:
    static TileIndexMapMap tileMaps;
};

#endif


