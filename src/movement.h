/**
 * $Id$
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "direction.h"

struct _Object;
struct _Map;

/* To handle being 'slowed' during movement */
typedef int (*SlowedCallback)(int);

int moveObject(struct _Map *map, struct _Object *obj, int avatarx, int avatary);
int moveCombatObject(int action, struct _Map *map, struct _Object *obj, int targetx, int targety);
int slowedHandlerDefault(int tile);
int slowedHandlerNone(int unused);
int slowedHandlerWind(int direction);

#endif
