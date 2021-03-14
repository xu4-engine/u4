/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include <vector>

#include "map.h"
#include "movement.h"
#include "observable.h"
#include "types.h"

typedef enum {
    CTX_WORLDMAP    = 0x0001,
    CTX_COMBAT      = 0x0002,
    CTX_CITY        = 0x0004,
    CTX_DUNGEON     = 0x0008,
    CTX_ALTAR_ROOM  = 0x0010,
    CTX_SHRINE      = 0x0020
} LocationContext;

#define CTX_ANY             (LocationContext)(0xffff)
#define CTX_NORMAL          (LocationContext)(CTX_WORLDMAP | CTX_CITY)
#define CTX_NON_COMBAT      (LocationContext)(CTX_ANY & ~CTX_COMBAT)
#define CTX_CAN_SAVE_GAME   (LocationContext)(CTX_WORLDMAP | CTX_DUNGEON)

class TurnCompleter;

class Location : public Observable<Location *, MoveEvent &> {
public:
    Location(MapCoords coords, Map *map, int viewmode, LocationContext ctx, TurnCompleter *turnCompleter, Location *prev);

    std::vector<MapTile> tilesAt(MapCoords coords, bool &focus);
    TileId getReplacementTile(MapCoords atCoords, Tile const * forTile);
    int getCurrentPosition(MapCoords *coords);
    MoveResult move(Direction dir, bool userEvent);

    MapCoords coords;
    Map *map;
    int viewMode;
    LocationContext context;
    TurnCompleter *turnCompleter;
    Location *prev;
};

void locationFree(Location **stack);

#endif
