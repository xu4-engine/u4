/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"
#include "../shrine.h"

const Shrine honesty_shrine;

Map shrine_honesty_map = {
    25,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &honesty_shrine } /* shrine */
};

const Shrine honesty_shrine = {
    VIRT_HONESTY, /* virtue */
    "ahm" /* mantra */
};

const Shrine compassion_shrine;

Map shrine_compassion_map = {
    26,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &compassion_shrine } /* shrine */
};

const Shrine compassion_shrine = {
    VIRT_COMPASSION, /* virtue */
    "mu" /* mantra */
};

const Shrine valor_shrine;

Map shrine_valor_map = {
    27,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &valor_shrine } /* shrine */
};

const Shrine valor_shrine = {
    VIRT_VALOR, /* virtue */
    "ra" /* mantra */
};

const Shrine justice_shrine;

Map shrine_justice_map = {
    28,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &justice_shrine } /* shrine */
};

const Shrine justice_shrine = {
    VIRT_JUSTICE, /* virtue */
    "beh" /* mantra */
};

const Shrine sacrifice_shrine;

Map shrine_sacrifice_map = {
    29,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &sacrifice_shrine } /* shrine */
};

const Shrine sacrifice_shrine = {
    VIRT_SACRIFICE, /* virtue */
    "cah" /* mantra */
};

const Shrine honor_shrine;

Map shrine_honor_map = {
    30,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &honor_shrine } /* shrine */
};

const Shrine honor_shrine = {
    VIRT_HONOR, /* virtue */
    "summ" /* mantra */
};

const Shrine spirituality_shrine;

Map shrine_spirituality_map = {
    31,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &spirituality_shrine } /* shrine */
};

const Shrine spirituality_shrine = {
    VIRT_SPIRITUALITY, /* virtue */
    "om" /* mantra */
};

const Shrine humility_shrine;

Map shrine_humility_map = {
    32,
    "shrine.con", /* fname */
    MAP_SHRINE, /* type */
    CON_WIDTH, CON_HEIGHT, /* width, height */
    5, 5, /* startx, starty */
    BORDER_FIXED, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* flags */
    MUSIC_SHRINES, /* music */
    NULL, /* data */
    { &humility_shrine } /* shrine */
};

const Shrine humility_shrine = {
    VIRT_HUMILITY, /* virtue */
    "lum" /* mantra */
};

