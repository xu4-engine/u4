#include <stdlib.h>

#include "location.h"

Location *locationNew(int x, int y, int z, Map *map, Location *prev) {
    Location *newLoc = (Location *)malloc(sizeof(Location));

    newLoc->x = x;
    newLoc->y = y;
    newLoc->z = z;
    newLoc->map = map;
    
    return locationPush(prev, newLoc);    
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
