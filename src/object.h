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
    unsigned char tile, prevtile;
    unsigned short x, y;
    unsigned short prevx, prevy;
    ObjectMovementBehavior movement_behavior;
    const struct _Person *person;
    int isAvatar;
    int hasFocus;
    struct _Object *next;
} Object;

#endif
