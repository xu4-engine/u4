/*
 * $Id$
 */

#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <map>
#include <string>

#include "map.h"

class U4FILE;
class Dungeon;

/**
 * The generic map loader interface.  Map loaders should override the
 * load method to load a map from the meta data already initialized in
 * the map object passed in. They must also register themselves with
 * registerLoader for one or more Map::Types.
 *
 * @todo
 * <ul>
 *      <li>
 *          Instead of loading dungeon room data into a u4dos-style structure and converting it to
 *          an xu4 Map when it's needed, convert it to an xu4 Map immediately upon loading it.
 *      </li>
 * </ul>
 */
class MapLoader {
public:
    virtual ~MapLoader() {}

    static MapLoader *getLoader(Map::Type type);

    virtual bool load(Map *map) = 0;

protected:
    static MapLoader *registerLoader(MapLoader *loader, Map::Type type);
    static bool loadData(Map *map, U4FILE *f);
    static bool isChunkCompressed(Map *map, int chunk);

private:
    static std::map<Map::Type, MapLoader *> *loaderMap;
};

class CityMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

class ConMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

class DngMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

private:
    void initDungeonRoom(Dungeon *dng, int room);
};

class WorldMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

#endif /* MAPLOADER_H */
