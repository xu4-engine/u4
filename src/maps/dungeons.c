/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"
#include "../dungeon.h"

Dungeon deceit_dungeon;

Map deceit_map = {
    17,
    "deceit.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &deceit_dungeon } /* dungeon */
};

Dungeon deceit_dungeon = {
    "Deceit", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon despise_dungeon;

Map despise_map = {
    18,
    "despise.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &despise_dungeon } /* dungeon */
};

Dungeon despise_dungeon = {
    "Despise", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon destard_dungeon;

Map destard_map = {
    19,
    "destard.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &destard_dungeon } /* dungeon */
};

Dungeon destard_dungeon = {
    "Destard", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon wrong_dungeon;

Map wrong_map = {
    20,
    "wrong.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &wrong_dungeon } /* dungeon */
};

Dungeon wrong_dungeon = {
    "Wrong", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon covetous_dungeon;

Map covetous_map = {
    21,
    "covetous.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &covetous_dungeon } /* dungeon */
};

Dungeon covetous_dungeon = {
    "Covetous", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon shame_dungeon;

Map shame_map = {
    22,
    "shame.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &shame_dungeon } /* dungeon */
};

Dungeon shame_dungeon = {
    "Shame", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon hythloth_dungeon;

Map hythloth_map = {
    23,
    "hythloth.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &hythloth_dungeon } /* dungeon */
};

Dungeon hythloth_dungeon = {
    "Hythloth", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

Dungeon abyss_dungeon;

Map abyss_map = {
    24,
    "abyss.dng", /* fname */
    MAP_DUNGEON, /* type */
    DNG_WIDTH, DNG_HEIGHT, 8, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    FIRST_PERSON, /* flags */
    MUSIC_DUNGEON, /* music */
    NULL, /* data */
    { &abyss_dungeon } /* dungeon */
};

Dungeon abyss_dungeon = {
    "The Great Stygian Abyss", /* name */
    0, /* n_rooms */
    NULL /* rooms */
};

