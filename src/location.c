/*
 * $Id$
 */

#include <stdlib.h>

#include "location.h"

Location *locationPush(Location *stack, Location *loc);
Location *locationPop(Location **stack);

/**
 * Add a new location to the stack, or 
 * start a new stack if 'prev' is NULL
 */

Location *locationNew(int x, int y, int z, Map *map, Location *prev) {
    Location *newLoc = (Location *)malloc(sizeof(Location));

    newLoc->x = x;
    newLoc->y = y;
    newLoc->z = z;
    newLoc->map = map;
    
    return locationPush(prev, newLoc);    
}

/**
 * Pop a location from the stack and free the memory
 */

void locationFree(Location **stack) {
    free(locationPop(stack));
}

Location *locationPush(Location *stack, Location *loc) {
    loc->prev = stack;
    return loc;
}

Location *locationPop(Location **stack) {
    Location *loc = *stack;    
    *stack = (*stack)->prev;
    loc->prev = NULL;
    return loc;
}
