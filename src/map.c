#include <stdlib.h>

#include "u4.h"
#include "map.h"
#include "annotation.h"

#define MAP_TILE_AT(mapptr, x, y) ((mapptr)->data[(x) + ((y) * (mapptr)->width)])

const Person *mapPersonAt(const Map *map, int x, int y) {
    int i;

    for(i = 0; i < map->n_persons; i++) {
	if (map->persons[i].startx == x &&
	    map->persons[i].starty == y) {
	    return &(map->persons[i]);
	}
    }
    return NULL;
}

const Portal *mapPortalAt(const Map *map, int x, int y) {
    int i;

    for(i = 0; i < map->n_portals; i++) {
        if (map->portals[i].x == x &&
	    map->portals[i].y == y) {
	    return &(map->portals[i]);
	}
    }
    return NULL;
}

unsigned char mapTileAt(const Map *map, int x, int y) {
    unsigned char tile;
    const Annotation *a;
 
    tile = MAP_TILE_AT(map, x, y);
    if ((a = annotationAt(x, y)) != NULL)
        tile = a->tile;
    
    return tile;
}
