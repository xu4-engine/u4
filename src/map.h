/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include "u4file.h"
#include "music.h"
#include "direction.h"
#include "types.h"

class AnnotationMgr;
struct _City;
struct _Shrine;
struct _Area;
struct _Person;
struct _Monster;
struct _Portal;
struct _Dungeon;

typedef unsigned char MapTile;
typedef unsigned char MapId;
typedef xu4_vector<struct _Portal*> PortalList;
typedef xu4_list<int> CompressedChunkList;

typedef enum {
    MAPTYPE_WORLD,
    MAPTYPE_TOWN,
    MAPTYPE_VILLAGE,
    MAPTYPE_CASTLE,
    MAPTYPE_RUIN,
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
#define WITHOUT_OBJECTS 0
#define WITH_GROUND_OBJECTS 1
#define WITH_OBJECTS 2

typedef struct _Map {
    MapId id;
    string fname;
    MapType type;
    unsigned int width, height, levels;
    unsigned int chunk_width, chunk_height;
    CompressedChunkList compressed_chunks;    
    MapBorderBehavior border_behavior;    
    PortalList portals;
    AnnotationMgr *annotations;
    int flags;
    Music music;
    xu4_vector<MapTile> data;
    union {
        void *init;
        struct _City *city;
        struct _Shrine *shrine;
        struct _Area *area;
        struct _Dungeon *dungeon;
    };
    ObjectList objects;
} Map;

/**
 * MapCoords class
 */ 
class MapCoords {    
public:
    int x, y, z;    
    
    MapCoords(int initx = 0, int inity = 0, int initz = 0) : 
        x(initx), y(inity), z(initz) {}
    
    bool operator==(const MapCoords &a) const {        
        return ((x == a.x) && (y == a.y) && (z == a.z)) ? true : false;        
    }
    bool operator!=(const MapCoords &a) const {
        return !operator==(a);
    }    
    
    MapCoords &wrap(const Map *map) {
        if (map && map->border_behavior == BORDER_WRAP) {
            while (x < 0)
                x += map->width;
            while (y < 0)
                y += map->height;
            while (x >= (int)map->width)
                x -= map->width;
            while (y >= (int)map->height)
                y -= map->height;
        }
        return *this;
    }

    MapCoords &putInBounds(const Map *map) {
        if (map) {
            if (x < 0)
                x = 0;
            if (x >= (int) map->width)
                x = map->width - 1;
            if (y < 0)
                y = 0;
            if (y >= (int) map->height)
                y = map->height - 1;
            if (z < 0)
                z = 0;
            if (z >= (int) map->levels)
                z = map->levels - 1;
        }
        return *this;
    }
    
    MapCoords &move(Direction d, const Map *map = NULL) {
        switch(d) {
        case DIR_NORTH: y--; break;
        case DIR_EAST: x++; break;
        case DIR_SOUTH: y++; break;
        case DIR_WEST: x--; break;
        default: break;
        }
        
        // Wrap the coordinates if necessary
        wrap(map);

        return *this;
    }

    MapCoords &move(int dx, int dy, const Map *map = NULL) {
        x += dx;
        y += dy;        
        
        // Wrap the coordinates if necessary
        wrap(map);

        return *this;
    }

    /**
     * Returns a mask of directions that indicate where one point is relative
     * to another.  For instance, if the object at (x, y) is
     * northeast of (c.x, c.y), then this function returns
     * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
     * This function also takes into account map boundaries and adjusts
     * itself accordingly. If the two coordinates are not on the same z-plane,
     * then this function return DIR_NONE.
     */
    int getRelativeDirection(const MapCoords &c, const Map *map = NULL) const {
        int dx, dy, dirmask;        
    
        dirmask = DIR_NONE;
        if (z != c.z)
            return dirmask;
        
        /* adjust our coordinates to find the closest path */
        if (map && map->border_behavior == BORDER_WRAP) {
            MapCoords me = *this;            
            
            if (abs(me.x - c.x) > abs(me.x + map->width - c.x))
                me.x += map->width;
            else if (abs(me.x - c.x) > abs(me.x - map->width - c.x))
                me.x -= map->width;

            if (abs(me.y - c.y) > abs(me.y + map->width - c.y))
                me.y += map->height;
            else if (abs(me.y - c.y) > abs(me.y - map->width - c.y))
                me.y -= map->height;

            dx = me.x - c.x;
            dy = me.y - c.y;
        }
        else {        
            dx = x - c.x;
            dy = y - c.y;
        }

        /* add x directions that lead towards to_x to the mask */
        if (dx < 0)         dirmask |= MASK_DIR(DIR_EAST);
        else if (dx > 0)    dirmask |= MASK_DIR(DIR_WEST);

        /* add y directions that lead towards to_y to the mask */
        if (dy < 0)         dirmask |= MASK_DIR(DIR_SOUTH);
        else if (dy > 0)    dirmask |= MASK_DIR(DIR_NORTH);

        /* return the result */
        return dirmask;
    }

    /**
     * Finds the appropriate direction to travel to get from one point to
     * another.  This algorithm will avoid getting trapped behind simple
     * obstacles, but still fails with anything mildly complicated.
     * This function also takes into account map boundaries and adjusts
     * itself accordingly, provided the 'map' parameter is passed
     */
    Direction pathTo(const MapCoords &c, int valid_directions = MASK_DIR_ALL, bool towards = true, const Map *map = NULL) const {
        int directionsToObject;
    
        /* find the directions that lead [to/away from] our target */
        directionsToObject = towards ? getRelativeDirection(c, map) : ~getRelativeDirection(c, map);
    
        /* make sure we eliminate impossible options */
        directionsToObject &= valid_directions;

        /* get the new direction to move */
        if (directionsToObject > DIR_NONE)
            return dirRandomDir(directionsToObject);
    
        /* there are no valid directions that lead to our target, just move wherever we can! */
        else return dirRandomDir(valid_directions);
    }

    /**
     * Finds the appropriate direction to travel to move away from one point
     */
    Direction pathAway(const MapCoords &c, int valid_directions = MASK_DIR_ALL) const {
        return pathTo(c, valid_directions, false);
    }
    
    /**
     * Finds the movement distance (not using diagonals) from point a to point b
     * on a map, taking into account map boundaries and such.  If the two coords
     * are not on the same z-plane, then this function returns -1;
     */
    int movementDistance(const MapCoords &c, const Map *map = NULL) const {
        int dirmask = DIR_NONE;
        int dist = 0;
        MapCoords me = *this;

        if (z != c.z)
            return -1;

        /* get the direction(s) to the coordinates */
        dirmask = getRelativeDirection(c, map);

        while ((me.x != c.x) || (me.y != c.y))
        {
            if (me.x != c.x) {
                if (dirmask & MASK_DIR_WEST)
                    me.move(DIR_WEST, map);
                else me.move(DIR_EAST, map);

                dist++;
            }
            if (me.y != c.y) {
                if (dirmask & MASK_DIR_NORTH)
                    me.move(DIR_NORTH, map);
                else me.move(DIR_SOUTH, map);

                dist++;
            }            
        }

        return dist;
    }

    /**
     * Finds the distance (using diagonals) from point a to point b on a map
     * If the two coordinates are not on the same z-plane, then this function
     * returns -1. This function also takes into account map boundaries.
     */ 
    int distance(const MapCoords &c, const Map *map = NULL) const {
        int dist = movementDistance(c, map);
        if (dist <= 0)
            return dist;

        /* calculate how many fewer movements there would have been */
        dist -= abs(x - c.x) < abs(y - c.y) ? abs(x - c.x) : abs(y - c.y);

        return dist;
    }
};

#define MAP_IS_OOB(mapptr, c) (((c).x) < 0 || ((c).x) >= ((int)(mapptr)->width) || ((c).y) < 0 || ((c).y) >= ((int)(mapptr)->height) || ((c).z) < 0 || ((c).z) >= ((int)(mapptr)->levels))

class Object *mapObjectAt(const Map *map, MapCoords coords);
const struct _Person *mapPersonAt(const Map *map, MapCoords coords);
const struct _Portal *mapPortalAt(const Map *map, MapCoords coords, int actionFlags);
MapTile mapGetTileFromData(const Map *map, MapCoords coords);
MapTile mapTileAt(const Map *map, MapCoords coords, int withObjects);
int mapIsWorldMap(const Map *map);
class Object *mapAddPersonObject(Map *map, const struct _Person *person);
class Object *mapAddMonsterObject(Map *map, const struct _Monster *monster, MapCoords coords);
class Object *mapAddObject(Map *map, MapTile tile, MapTile prevtile, MapCoords coords);
void mapRemoveObject(Map *map, Object *rem);
ObjectList::iterator mapRemoveObject(Map *map, ObjectList::iterator rem);
void mapRemovePerson(Map *map, const struct _Person *person);
void mapClearObjects(Map *map);
class Object *mapMoveObjects(Map *map, MapCoords avatar);
void mapAnimateObjects(Map *map);
void mapResetObjectAnimations(Map *map);
int mapNumberOfMonsters(const Map *map);
int mapGetValidMoves(const Map *map, MapCoords from, MapTile transport);

#endif
