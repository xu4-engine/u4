/*
 * $Id$
 */

#ifndef DUNGEON_H
#define DUNGEON_H

struct _Map;
struct _DngRoom;

typedef struct _Dungeon {
    const char *name;
    int n_rooms;
    struct _Map *room;
    struct _DngRoom *rooms;
    unsigned char party_startx[8];
    unsigned char party_starty[8];
} Dungeon;

#endif
