/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include "map.h"

/*typedef void (*MapSetTile)(Map *map, int x, int y, int z, unsigned char tile);
typedef void (*MapGetTile)(Map *map, int x, int y, int z);*/
typedef unsigned char (*TileAt)(const Map *map, int x, int y, int z, int withObjects);

typedef enum {
    CTX_WORLDMAP    = 0x0001,
    CTX_COMBAT      = 0x0002,
    CTX_CITY        = 0x0004,
    CTX_DUNGEON     = 0x0008
} LocationContext;

/*typedef struct _TileFunctions {
    MapSetTile mapSetTile;
    MapGetTile mapGetTile;
    MapTileAt mapTileAt;
} TileFunctions;*/

#define CTX_ANY             (0xffff)
#define CTX_NORMAL          (CTX_WORLDMAP | CTX_CITY)
#define CTX_NON_COMBAT      (CTX_ANY & ~CTX_COMBAT)
/* FIXME: need to be able to save in dungeons */
//#define CTX_CAN_SAVE_GAME   (CTX_WORLDMAP | CTX_DUNGEON)
#define CTX_CAN_SAVE_GAME   (CTX_WORLDMAP)

typedef void (*FinishTurnCallback)(void);
typedef int (*MoveCallback)(Direction, int);

typedef struct _Location {
    int x;
    int y;
    int z;	
	Map *map;
    int viewMode;
    LocationContext context;
    FinishTurnCallback finishTurn;
    MoveCallback move;
    TileAt tileAt;
    struct _Location *prev;
} Location;

Location *locationNew(int x, int y, int z, Map *map, int viewmode, LocationContext ctx, FinishTurnCallback finishTurnCallback, MoveCallback moveCallback, TileAt tileAtCallback, Location *prev);
unsigned char locationVisibleTileAt(Location *location, int x, int y, int z, int *focus);
int locationGetCurrentPosition(Location *location, int *x, int *y, int *z);
void locationFree(Location **stack);

#endif
