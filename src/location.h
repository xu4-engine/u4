#ifndef LOCATION_H
#define LOCATION_H

#include "map.h"

typedef struct _Location {
    int x;
    int y;
    int z;	
	Map *map;
    struct _Location *prev;
} Location;

Location *locationNew(int x, int y, int z, Map *map, Location *prev);
Location *locationPush(Location *stack, Location *loc);
Location *locationPop(Location **stack);

#endif