/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include <stdio.h>

#include "portal.h"
#include "person.h"

typedef enum {
    BORDER_WRAP,
    BORDER_EXIT2PARENT,
    BORDER_FIXED
} MapBorderBehavior;

/* flags */
#define SHOW_AVATAR (1 << 0)

typedef struct _Map {
    const char *name;
    unsigned int width, height;
    unsigned int startx, starty;
    MapBorderBehavior border_behavior;
    int n_portals;
    const Portal *portals;
    int n_persons;
    Person *persons;
    int flags;
    unsigned char *data;
    const char *ult_fname;
    const char *tlk_fname;
} Map;

#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= (mapptr)->width || (y) < 0 || (y) >= (mapptr)->height)

int mapRead(Map *map, FILE *ult, FILE *tlk);
int mapReadWorld(Map *map, FILE *world);
const Person *mapPersonAt(const Map *map, int x, int y);
const Portal *mapPortalAt(const Map *map, int x, int y);
unsigned char mapTileAt(const Map *map, int x, int y);

#endif
