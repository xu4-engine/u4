/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"

Map brick_map = {
    33,
    "brick.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map bridge_map = {
    33,
    "bridge.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map brush_map = {
    33,
    "brush.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map camp_map = {
    33,
    "camp.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng0_map = {
    33,
    "dng0.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng1_map = {
    33,
    "dng1.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng2_map = {
    33,
    "dng2.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng3_map = {
    33,
    "dng3.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng4_map = {
    33,
    "dng4.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng5_map = {
    33,
    "dng5.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dng6_map = {
    33,
    "dng6.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map dungeon_map = {
    33,
    "dungeon.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map forest_map = {
    33,
    "forest.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map grass_map = {
    33,
    "grass.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map hill_map = {
    33,
    "hill.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map inn_map = {
    33,
    "inn.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map marsh_map = {
    33,
    "marsh.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map shipsea_map = {
    33,
    "shipsea.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map shipship_map = {
    33,
    "shipship.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map shipshor_map = {
    33,
    "shipshor.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map shore_map = {
    33,
    "shore.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};

Map shorship_map = {
    33,
    "shorship.con", /* fname */
    MAP_COMBAT, /* type */
    CON_WIDTH, CON_HEIGHT, 1, /* width, height, levels */
    5, 5, 0, /* startx, starty, startlevel */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    NO_LINE_OF_SIGHT, /* flags */
    MUSIC_COMBAT, /* music */
    NULL, /* data */
    { NULL } /* city */
};
