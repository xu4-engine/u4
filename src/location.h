/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include "map.h"

typedef void (*FinishTurnCallback)(void);

typedef struct _Location {
    int x;
    int y;
    int z;	
	Map *map;
    int viewMode;
    FinishTurnCallback finishTurn;
    struct _Location *prev;
} Location;

Location *locationNew(int x, int y, int z, Map *map, int viewmode, FinishTurnCallback callback, Location *prev);
void locationFree(Location **stack);

#endif
