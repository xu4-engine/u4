/**
 * $Id$
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "direction.h"

struct _Object;
struct _Map;

typedef enum {
    SLOWED_BY_NOTHING,
    SLOWED_BY_TILE,
    SLOWED_BY_WIND
} SlowedType;

int moveObject(struct _Map *map, struct _Object *obj, int avatarx, int avatary);
int moveCombatObject(int action, struct _Map *map, struct _Object *obj, int targetx, int targety);
int slowedByTile(unsigned char tile);
int slowedByWind(int direction);

#endif
