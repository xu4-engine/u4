/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <stdio.h>

struct _City;
struct _Shrine;
struct _Con;
struct _Object;
struct _Person;
struct _Portal;
struct _Annotation;

#include "music.h"

typedef enum {
    MAP_WORLD,
    MAP_TOWN,
    MAP_VILLAGE,
    MAP_CASTLE,
    MAP_RUIN,
    MAP_SHRINE,
    MAP_COMBAT
} MapType;

typedef enum {
    BORDER_WRAP,
    BORDER_EXIT2PARENT,
    BORDER_FIXED
} MapBorderBehavior;

/* flags */
#define SHOW_AVATAR (1 << 0)
#define NO_LINE_OF_SIGHT (1 << 1)

typedef struct _Map {
    const char *fname;
    MapType type;
    unsigned int width, height;
    unsigned int startx, starty;
    MapBorderBehavior border_behavior;
    int n_portals;
    const struct _Portal *portals;
    int flags;
    Music music;
    unsigned char *data;
    union {
        struct _City *city;
        const struct _Shrine *shrine;
        struct _Con *con;
    };
    struct _Annotation *annotation;
    struct _Object *objects;
} Map;

#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= ((int)(mapptr)->width) || (y) < 0 || (y) >= ((int)(mapptr)->height))

int mapRead(struct _City *city, FILE *ult, FILE *tlk);
int mapReadCon(Map *map, FILE *con);
int mapReadWorld(Map *map, FILE *world);
struct _Object *mapObjectAt(const Map *map, int x, int y, int ignoreAvatar);
const struct _Person *mapPersonAt(const Map *map, int x, int y);
const struct _Portal *mapPortalAt(const Map *map, int x, int y);
unsigned char mapTileAt(const Map *map, int x, int y);
int mapIsWorldMap(const Map *map);
void mapAddPersonObject(Map *map, const struct _Person *person);
void mapAddObject(Map *map, unsigned int tile, unsigned int prevtile, unsigned short x, unsigned short y);
void mapAddAvatarObject(Map *map, unsigned int tile, unsigned short x, unsigned short y);
void mapRemoveObject(Map *map, struct _Object *obj);
void mapRemoveAvatarObject(Map *map);
void mapRemovePerson(Map *map, const struct _Person *person);
void mapClearObjects(Map *map);
void mapMoveObjects(Map *map, int avatarx, int avatary);
void mapAnimateObjects(Map *map);

#endif
