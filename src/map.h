/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

struct _City;
struct _Shrine;
struct _Area;
struct _Object;
struct _Person;
struct _Monster;
struct _Portal;
struct _Dungeon;

#include "u4file.h"
#include "music.h"
#include "direction.h"

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

typedef struct _Map {
    unsigned char id;
    const char *fname;
    MapType type;
    unsigned int width, height, levels;
    MapBorderBehavior border_behavior;
    int n_portals;
    struct _Portal *portals;
    int flags;
    Music music;
    unsigned char *data;
    union {
        void *init;
        struct _City *city;
        struct _Shrine *shrine;
        struct _Area *area;
        struct _Dungeon *dungeon;
    };
    struct _Object *objects;
} Map;

typedef struct _Trigger {
    unsigned char tile;
    unsigned char
        x : 4,
        y : 4,
        change_x1 : 4,
        change_y1 : 4,
        change_x2 : 4,
        change_y2 : 4;
} Trigger;

typedef struct _DngRoom {
    Trigger triggers[4];
    unsigned char monster_tiles[16];
    unsigned char monster_start_x[16];
    unsigned char monster_start_y[16];
    unsigned char party_north_start_x[8];
    unsigned char party_north_start_y[8];
    unsigned char party_east_start_x[8];
    unsigned char party_east_start_y[8];
    unsigned char party_south_start_x[8];
    unsigned char party_south_start_y[8];
    unsigned char party_west_start_x[8];
    unsigned char party_west_start_y[8];
    unsigned char map_data[121];
    unsigned char buffer[7];
} DngRoom;

#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= ((int)(mapptr)->width) || (y) < 0 || (y) >= ((int)(mapptr)->height))

int mapRead(struct _City *city, U4FILE *ult, U4FILE *tlk);
int mapReadCon(Map *map, U4FILE *con);
int mapReadDng(Map *map, U4FILE *dng);
int mapReadWorld(Map *map, U4FILE *world);
struct _Object *mapObjectAt(const Map *map, int x, int y, int z);
const struct _Person *mapPersonAt(const Map *map, int x, int y, int z);
const struct _Portal *mapPortalAt(const Map *map, int x, int y, int z, int actionFlags);
unsigned char mapGetTileFromData(const Map *map, int x, int y, int z);
unsigned char mapTileAt(const Map *map, int x, int y, int z);
unsigned char mapGroundTileAt(const Map *map, int x, int y, int z);
unsigned char mapDungeonTileAt(const Map *map, int x, int y, int z);
int mapLoadDungeonRoom(struct _Dungeon *dng, int room);
int mapIsWorldMap(const Map *map);
struct _Object *mapAddPersonObject(Map *map, const struct _Person *person);
struct _Object *mapAddMonsterObject(Map *map, const struct _Monster *monster, int x, int y, int z);
struct _Object *mapAddObject(Map *map, unsigned char tile, unsigned char prevtile, int x, int y, int z);
void mapRemoveObject(Map *map, struct _Object *obj);
void mapRemovePerson(Map *map, const struct _Person *person);
void mapClearObjects(Map *map);
struct _Object *mapMoveObjects(Map *map, int avatarx, int avatary, int z);
void mapAnimateObjects(Map *map);
void mapResetObjectAnimations(Map *map);
int mapNumberOfMonsters(const Map *map);
int mapGetValidMoves(const Map *map, int from_x, int from_y, int z, unsigned char transport);
int mapDistance(int x1, int y1, int x2, int y2);
int mapMovementDistance(int x1, int y1, int x2, int y2);
int mapDirMove(const Map *map, Direction dir, int *x, int *y);
int mapWrapCoordinates(const Map *map, int *x, int *y);
int mapIsObstructed(const Map *map, int x, int y, int z, Direction dir, int distance);

#endif
