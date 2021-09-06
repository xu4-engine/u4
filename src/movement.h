/**
 * $Id$
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "direction.h"
#include "map.h"

class Object;
class Map;
class Tile;

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

class MoveEvent {
public:
    MoveEvent(Direction d, bool user) : dir(d), userEvent(user), result(MOVE_SUCCEEDED) {}

    Direction dir;              /**< the direction of the move */
    bool userEvent;             /**< whether the user initiated the move */
    MoveResult result;          /**< how the movement was resolved */
};

void moveAvatar(MoveEvent &event);
void moveAvatarInDungeon(MoveEvent &event);
int moveObject(class Map *map, class Creature *obj, const Coords& avatar);
int moveCombatObject(int action, class Map *map, class Creature *obj, const Coords& target);
void movePartyMember(MoveEvent &event);
bool slowedByTile(const Tile *tile);
bool slowedByWind(int direction);

extern bool collisionOverride;

#endif
