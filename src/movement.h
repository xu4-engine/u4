/**
 * $Id$
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "direction.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

class Object;
struct _Map;

typedef enum {
    SLOWED_BY_NOTHING,
    SLOWED_BY_TILE,
    SLOWED_BY_WIND
} SlowedType;

typedef enum {
    MOVE_SUCCEEDED          = 0x0001,    
    MOVE_END_TURN           = 0x0002,
    MOVE_BLOCKED            = 0x0004,
    MOVE_MAP_CHANGE         = 0x0008,
    MOVE_TURNED             = 0x0010,  /* dungeons and ship movement */
    MOVE_DRIFT_ONLY         = 0x0020,  /* balloon -- no movement */
    MOVE_EXIT_TO_PARENT     = 0x0040,
    MOVE_SLOWED             = 0x0080,
    MOVE_MUST_USE_SAME_EXIT = 0x0100
} MoveReturnValue;

typedef MoveReturnValue (*MoveCallback)(Direction, int);

MoveReturnValue moveAvatar(Direction dir, int userEvent);
MoveReturnValue moveAvatarInDungeon(Direction dir, int userEvent);
int moveObject(struct _Map *map, class Object *obj, MapCoords avatar);
int moveCombatObject(int action, struct _Map *map, class Object *obj, MapCoords target);
MoveReturnValue movePartyMember(Direction dir, int userEvent);
int slowedByTile(MapTile tile);
int slowedByWind(int direction);

extern int collisionOverride;

#ifdef __cplusplus
}
#endif

#endif
