/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"

Map empath_map = {
    "Empath Abbey", /* name */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    15, 31, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* n_persons */
    NULL, /* persons */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    "empath.ult", /* ult_fname */
    "empath.tlk" /* tlk_fname */
};

Map lycaeum_map = {
    "Lycaeum", /* name */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    15, 31, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* n_persons */
    NULL, /* persons */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    "lycaeum.ult", /* ult_fname */
    "lycaeum.tlk" /* tlk_fname */
};

Map serpent_map = {
    "serpent", /* name */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    15, 31, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    0, /* n_persons */
    NULL, /* persons */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    "serpent.ult", /* ult_fname */
    "serpent.tlk" /* tlk_fname */
};

Map lcb_2_map;

const Portal lcb_1_portals[] = {
    { 3, 3, &lcb_2_map, ACTION_KLIMB },
    { 27, 3, &lcb_2_map, ACTION_KLIMB }
};

Map lcb_1_map = {
    "Castle of Lord British", /* name */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    15, 31, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    2, /* n_portals */
    lcb_1_portals, /* portals */
    0, /* n_persons */
    NULL, /* persons */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    "lcb_1.ult", /* ult_fname */
    "lcb.tlk" /* tlk_fname */
};

const Portal lcb_2_portals[] = {
    { 3, 3, &lcb_1_map, ACTION_DESCEND },
    { 27, 3, &lcb_1_map, ACTION_DESCEND }
};

Map lcb_2_map = {
    "Castle of Lord British", /* name */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    15, 31, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    2, /* n_portals */
    lcb_2_portals, /* portals */
    0, /* n_persons */
    NULL, /* persons */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    "lcb_2.ult", /* ult_fname */
    "lcb.tlk" /* tlk_fname */
};
