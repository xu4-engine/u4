/*
 * $Id$
 */

#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <map>
#include <string>

#include "map.h"

class U4FILE;

/**
 * The generic map loader interface.  Map loaders should override the
 * load method to load a map from the meta data already initialized in
 * the map object passed in. They must also register themselves with
 * registerLoader for one or more MapTypes.
 */
class MapLoader {
public:
    virtual int load(Map *map) = 0;
    static MapLoader *getLoader(MapType type);

protected:
    static MapLoader *registerLoader(MapLoader *loader, MapType type);
    static int loadData(Map *map, U4FILE *f);
    static int isChunkCompressed(Map *map, int chunk);

private:
    static std::map<MapType, MapLoader *> *loaderMap;
};

class CityMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual int load(Map *map);

};

class ConMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual int load(Map *map);

};

class DngMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual int load(Map *map);

};

class WorldMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual int load(Map *map);

};

#endif /* MAPLOADER_H */
