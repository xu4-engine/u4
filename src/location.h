/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include <vector>

#include "coords.h"
#include "direction.h"
#include "types.h"

enum SlowedType {
    SLOWED_BY_NOTHING,
    SLOWED_BY_TILE,
    SLOWED_BY_WIND
};

enum MoveResult {
    MOVE_SUCCEEDED          = 0x0001,
    MOVE_END_TURN           = 0x0002,
    MOVE_BLOCKED            = 0x0004,
    MOVE_MAP_CHANGE         = 0x0008,
    MOVE_TURNED             = 0x0010,  /* dungeons and ship movement */
    MOVE_DRIFT_ONLY         = 0x0020,  /* balloon -- no movement */
    MOVE_EXIT_TO_PARENT     = 0x0040,
    MOVE_SLOWED             = 0x0080,
    MOVE_MUST_USE_SAME_EXIT = 0x0100
};

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

class Creature;
class Map;
class Tile;
class TurnController;

class Location {
public:
    Location(const Coords& coords, Map *map, int viewmode, LocationContext ctx, TurnController *turnCompleter, Location *prev);

    void getTilesAt(std::vector<MapTile>& tiles, const Coords& coords,
                    bool& focus);
    TileId getReplacementTile(const Coords& atCoords, Tile const * forTile);
    int getCurrentPosition(Coords * pos);
    MoveResult move(Direction dir, bool userEvent);

    Coords coords;
    Map *map;
    int viewMode;
    LocationContext context;
    TurnController *turnCompleter;
    Location *prev;
};

class MoveEvent {
public:
    MoveEvent(Direction d, bool user) :
        dir(d), userEvent(user), result(MOVE_SUCCEEDED) {}

    Location* location;
    Direction dir;              /**< the direction of the move */
    bool userEvent;             /**< whether the user initiated the move */
    MoveResult result;          /**< how the movement was resolved */
};

extern bool collisionOverride;

void locationFree(Location **stack);
int moveCombatObject(int act, Map *map, Creature *obj, const Coords& target);
int moveObject(Map *map, Creature *obj, const Coords& avatar);

#endif
