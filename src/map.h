/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <stdio.h>

struct _City;
struct _Portal;

#include "person.h"

typedef enum {
    BORDER_WRAP,
    BORDER_EXIT2PARENT,
    BORDER_FIXED
} MapBorderBehavior;

/* flags */
#define SHOW_AVATAR (1 << 0)

typedef struct _Map {
    const char *fname;
    unsigned int width, height;
    unsigned int startx, starty;
    MapBorderBehavior border_behavior;
    int n_portals;
    const struct _Portal *portals;
    int flags;
    unsigned char *data;
    struct _City *city;
} Map;

#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= ((int)(mapptr)->width) || (y) < 0 || (y) >= ((int)(mapptr)->height))

int mapRead(struct _City *city, FILE *ult, FILE *tlk);
int mapReadCon(Map *map, FILE *con, int header);
int mapReadWorld(Map *map, FILE *world);
const Person *mapPersonAt(const Map *map, int x, int y);
const struct _Portal *mapPortalAt(const Map *map, int x, int y);
unsigned char mapTileAt(const Map *map, int x, int y);
int mapIsWorldMap(const Map *map);

#endif
