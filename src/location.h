/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include "map.h"

typedef struct _Location {
    int x;
    int y;
    int z;	
	Map *map;
    int viewMode;
    struct _Location *prev;
} Location;

Location *locationNew(int x, int y, int z, Map *map, int viewmode, Location *prev);
void locationFree(Location **stack);

#endif
