/*
 * $Id$
 */

#ifndef DUNGEON_H
#define DUNGEON_H

struct _Map;

typedef struct _Trigger {
    unsigned char tile;
    unsigned char
        x : 4,
        y : 4,
        change_x1 : 4,
        change_y1 : 4,
        change_x2 : 4,
        change_y2 : 4;
} Trigger;

typedef struct _DngRoom {
    Trigger triggers[4];
    unsigned char monster_tiles[16];
    unsigned char monster_start_x[16];
    unsigned char monster_start_y[16];
    unsigned char party_north_start_x[8];
    unsigned char party_north_start_y[8];
    unsigned char party_east_start_x[8];
    unsigned char party_east_start_y[8];
    unsigned char party_south_start_x[8];
    unsigned char party_south_start_y[8];
    unsigned char party_west_start_x[8];
    unsigned char party_west_start_y[8];
    unsigned char map_data[121];
    unsigned char buffer[7];
} DngRoom;

typedef struct _Dungeon {
    const char *name;
    int n_rooms;
    struct _Map *room;
    struct _DngRoom *rooms;
    unsigned char party_startx[8];
    unsigned char party_starty[8];
} Dungeon;

#endif
