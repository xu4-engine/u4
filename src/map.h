/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <list>
#include <string>
#include <vector>

#include "coords.h"
#include "direction.h"
#include "map.h"
#include "music.h"
#include "object.h"
#include "savegame.h"
#include "types.h"
#include "u4file.h"

using std::string;

#define MAP_IS_OOB(mapptr, c) (((c).x) < 0 || ((c).x) >= (static_cast<int>((mapptr)->width)) || ((c).y) < 0 || ((c).y) >= (static_cast<int>((mapptr)->height)) || ((c).z) < 0 || ((c).z) >= (static_cast<int>((mapptr)->levels)))

class AnnotationMgr;
class Map;
class Object;
class Person;
class Creature;
struct _Portal;
struct _Dungeon;

typedef std::vector<struct _Portal*> PortalList;
typedef std::list<int> CompressedChunkList;
typedef std::vector<MapTile> MapData;

typedef enum {
    MAPTYPE_WORLD,
    MAPTYPE_CITY,    
    MAPTYPE_SHRINE,
    MAPTYPE_COMBAT,
    MAPTYPE_DUNGEON
} MapType;

typedef enum {
    BORDER_WRAP,
    BORDER_EXIT2PARENT,
    BORDER_FIXED
} MapBorderBehavior;

/* flags */
#define SHOW_AVATAR (1 << 0)
#define NO_LINE_OF_SIGHT (1 << 1)
#define FIRST_PERSON (1 << 2)

/* mapTileAt flags */
#define WITHOUT_OBJECTS     0
#define WITH_GROUND_OBJECTS 1
#define WITH_OBJECTS        2

/**
 * MapCoords class
 */ 
class MapCoords : public Coords {    
public:
    MapCoords(int initx = 0, int inity = 0, int initz = 0) : Coords(initx, inity, initz) {}
    MapCoords(const Coords &a) : Coords(a.x, a.y, a.z) {}
    
    MapCoords &operator=(const Coords &a) { x = a.x; y = a.y; z = a.z; return *this; }
    bool operator==(const MapCoords &a) const;
    bool operator!=(const MapCoords &a) const;
    
    MapCoords &wrap(const class Map *map);
    MapCoords &putInBounds(const class Map *map);
    MapCoords &move(Direction d, const class Map *map = NULL);
    MapCoords &move(int dx, int dy, const class Map *map = NULL);    
    int getRelativeDirection(const MapCoords &c, const class Map *map = NULL) const;
    Direction pathTo(const MapCoords &c, int valid_dirs = MASK_DIR_ALL, bool towards = true, const class Map *map = NULL) const;
    Direction pathAway(const MapCoords &c, int valid_dirs = MASK_DIR_ALL) const;
    int movementDistance(const MapCoords &c, const class Map *map = NULL) const;
    int distance(const MapCoords &c, const class Map *map = NULL) const;
};

/**
 * Map class
 */ 
class Map {    
public:
    Map();
    Map(MapId id);
    virtual ~Map();

    // Member functions
    virtual string getName();
    
    class Object *objectAt(MapCoords coords);    
    const struct _Portal *portalAt(MapCoords coords, int actionFlags);
    MapTile* getTileFromData(Coords coords);
    MapTile* tileAt(Coords coords, int withObjects);
    bool isWorldMap();
    bool isEnclosed(Coords party);
    class Creature *addCreature(const class Creature *m, Coords coords);
    class Object *addObject(MapTile tile, MapTile prevTile, Coords coords);
    void removeObject(const class Object *rem);
    ObjectDeque::iterator removeObject(ObjectDeque::iterator rem);    
    void clearObjects();
    class Creature *moveObjects(MapCoords avatar);
    void resetObjectAnimations();
    int getNumberOfCreatures();
    int getValidMoves(MapCoords from, MapTile transport);
    bool move(Object *obj, Direction d);

    // u4dos compatibility
    bool fillMonsterTable();

public:
    MapId           id;
    string          fname;
    MapType         type;
    unsigned int    width,
                    height,
                    levels;
    unsigned int    chunk_width,
                    chunk_height;
    
    CompressedChunkList     compressed_chunks;
    MapBorderBehavior       border_behavior;

    PortalList      portals;
    AnnotationMgr  *annotations;
    int             flags;
    Music           music;
    MapData         data;
    ObjectDeque     objects;

    // u4dos compatibility
    SaveGameMonsterRecord monsterTable[MONSTERTABLE_SIZE];

private:
    void findWalkability(Coords coords, int *path_data);
};

#endif
