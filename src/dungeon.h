/*
 * $Id$
 */

#ifndef DUNGEON_H
#define DUNGEON_H

typedef struct _Dungeon {
    const char *name;
    int n_rooms;
    struct _Map *rooms;
} Dungeon;

#endif
