/*
 * $Id$
 */

#ifndef OBJECT_H
#define OBJECT_H

struct _Person;

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} ObjectMovementBehavior;

typedef struct _Object {
    unsigned int tile, prevtile;
    unsigned int x, y;
    ObjectMovementBehavior movement_behavior;
    const struct _Person *person;
    struct _Object *next;
} Object;

#endif
