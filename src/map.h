/*
 * $Id$
 */

#ifndef MAP_H
#define MAP_H

#include "portal.h"
#include "person.h"

typedef enum {
    BORDER_WRAP,
    BORDER_EXIT2PARENT,
    BORDER_FIXED
} MapBorderBehavior;

/* flags */
#define SHOW_AVATAR (1 << 0)

typedef struct MapTag {
    char *name;
    unsigned int width, height;
    unsigned int startx, starty;
    MapBorderBehavior border_behavior;
    int n_portals;
    const Portal *portals;
    int n_persons;
    const Person *persons;
    int flags;
    const unsigned char *data;
} Map;

#define MAP_TILE_AT(mapptr, x, y) ((mapptr)->data[(x) + ((y) * (mapptr)->width)])
#define MAP_IS_OOB(mapptr, x, y) ((x) < 0 || (x) >= (mapptr)->width || (y) < 0 || (y) >= (mapptr)->height)

const Person *mapPersonAt(const Map *map, int x, int y);
const Portal *mapPortalAt(const Map *map, int x, int y);

#endif
