/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <stdio.h>

struct _City;
struct _Shrine;
struct _Area;
struct _Object;
struct _Person;
struct _Portal;

#include "music.h"

typedef enum {
    MAP_WORLD,
    MAP_TOWN,
    MAP_VILLAGE,
    MAP_CASTLE,
    MAP_RUIN,
    MAP_SHRINE,
    MAP_COMBAT,
    MAP_DUNGEON
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
    const struct _Portal *portals;
    int flags;
    Music music;
    unsigned char *data;
    union {
        const void *init;
        struct _City *city;
        const struct _Shrine *shrine;
        struct _Area *area;
    };
    struct _Object *objects;
} Map;

#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= ((int)(mapptr)->width) || (y) < 0 || (y) >= ((int)(mapptr)->height))

int mapRead(struct _City *city, FILE *ult, FILE *tlk);
int mapReadCon(Map *map, FILE *con);
int mapReadDng(Map *map, FILE *dng);
int mapReadWorld(Map *map, FILE *world);
struct _Object *mapObjectAt(const Map *map, int x, int y, int z);
const struct _Person *mapPersonAt(const Map *map, int x, int y, int z);
const struct _Portal *mapPortalAt(const Map *map, int x, int y, int z);
unsigned char mapTileAt(const Map *map, int x, int y, int z);
unsigned char mapVisibleTileAt(const Map *map, int x, int y, int z, int *focus);
int mapIsWorldMap(const Map *map);
struct _Object *mapAddPersonObject(Map *map, const struct _Person *person);
struct _Object *mapAddObject(Map *map, unsigned int tile, unsigned int prevtile, unsigned short x, unsigned short y, unsigned short z);
void mapRemoveObject(Map *map, struct _Object *obj);
void mapRemovePerson(Map *map, const struct _Person *person);
void mapClearObjects(Map *map);
struct _Object *mapMoveObjects(Map *map, int avatarx, int avatary, int z);
void mapAnimateObjects(Map *map);
int mapNumberOfMonsters(const Map *map);
int mapGetValidMoves(const Map *map, int from_x, int from_y, int z, unsigned char transport);

#endif
