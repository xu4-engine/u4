/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"
#include "../shrine.h"

Shrine honesty_shrine;

Map shrine_honesty_map = {
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
    { .shrine = &honesty_shrine } /* shrine */
};

Shrine honesty_shrine = {
    VIRT_HONESTY, /* virtue */
    "ahm" /* mantra */
};

Shrine compassion_shrine;

Map shrine_compassion_map = {
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
    { .shrine = &compassion_shrine } /* shrine */
};

Shrine compassion_shrine = {
    VIRT_COMPASSION, /* virtue */
    "mu" /* mantra */
};

Shrine valor_shrine;

Map shrine_valor_map = {
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
    { .shrine = &valor_shrine } /* shrine */
};

Shrine valor_shrine = {
    VIRT_VALOR, /* virtue */
    "ra" /* mantra */
};

Shrine justice_shrine;

Map shrine_justice_map = {
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
    { .shrine = &justice_shrine } /* shrine */
};

Shrine justice_shrine = {
    VIRT_JUSTICE, /* virtue */
    "beh" /* mantra */
};

Shrine sacrifice_shrine;

Map shrine_sacrifice_map = {
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
    { .shrine = &sacrifice_shrine } /* shrine */
};

Shrine sacrifice_shrine = {
    VIRT_SACRIFICE, /* virtue */
    "cah" /* mantra */
};

Shrine honor_shrine;

Map shrine_honor_map = {
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
    { .shrine = &honor_shrine } /* shrine */
};

Shrine honor_shrine = {
    VIRT_HONOR, /* virtue */
    "summ" /* mantra */
};

Shrine humility_shrine;

Map shrine_humility_map = {
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
    { .shrine = &humility_shrine } /* shrine */
};

Shrine humility_shrine = {
    VIRT_HUMILITY, /* virtue */
    "lum" /* mantra */
};

